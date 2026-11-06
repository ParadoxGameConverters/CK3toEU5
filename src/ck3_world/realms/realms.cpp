#include "realms.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Log.h"
#include "src/ck3_world/characters/character.hpp"
#include "src/ck3_world/characters/characters.hpp"
#include "src/ck3_world/geography/county_details.hpp"
#include "src/ck3_world/titles/title.hpp"
#include "src/ck3_world/titles/titles.hpp"

namespace
{
using TitlePtr = std::shared_ptr<ck3::Title>;
using IdTitleMap = std::map<long long, TitlePtr>;

// Titles are stored under their key, but liege and capital references are by ID.
IdTitleMap MapTitlesById(const ck3::Titles& titles)
{
   IdTitleMap id_title_map;
   for (const auto& title: titles.GetTitles())
   {
      id_title_map.insert(std::pair(title.second->GetID(), title.second));
   }
   return id_title_map;
}

// Flagging de jure independence is not enough - duchies that break away in full independence would
// lose all their land. A title is independent when nothing holds it de facto, and a de facto liege
// that is itself unheld does not count, since CK3 leaves destroyed lieges behind in the save.
bool IsIndependent(const ck3::Title& title, const IdTitleMap& id_title_map)
{
   const auto& liege = title.GetDeFactoLiege();
   if (!liege.has_value())
   {
      return true;
   }
   const auto liege_title = id_title_map.find(liege->GetID());
   if (liege_title == id_title_map.end())
   {
      return true;
   }
   return !liege_title->second->GetHolder().has_value();
}

// Filters out landless titular titles - mercenary companies, holy orders, a landless Pope. Only the
// holder needs checking: a ruler holding a titular title alongside real land is independent anyway.
bool HoldsLand(const ck3::Character& character)
{
   if (!character.GetCharacterRealm().has_value())
   {
      return false;
   }
   for (const auto& domain_title: character.GetCharacterRealm()->GetDomain())
   {
      const auto held = domain_title.GetPointer().lock();
      if (held && (held->GetLevel() == ck3::Level::kCounty || held->GetLevel() == ck3::Level::kBarony))
      {
         return true;
      }
   }
   return false;
}

// The first entry of a character's domain is their primary title. Fall back to the highest tier one
// we found, for rulers whose primary title is somehow not among their independent titles.
TitlePtr SelectPrimaryTitle(const std::vector<TitlePtr>& held_titles, const ck3::Character& holder)
{
   if (holder.GetCharacterRealm().has_value() && !holder.GetCharacterRealm()->GetDomain().empty())
   {
      const auto primary = holder.GetCharacterRealm()->GetDomain().front().GetPointer().lock();
      if (primary && std::ranges::find(held_titles, primary) != held_titles.end())
      {
         return primary;
      }
   }
   const auto highest = std::ranges::max_element(held_titles, {}, [](const TitlePtr& title) {
      return title->GetLevel();
   });
   return highest == held_titles.end() ? nullptr : *highest;
}

// Walks the de facto vassal tree below the realm's top level titles, collecting every county.
void GatherCounties(const std::vector<TitlePtr>& held_titles,
    const std::map<long long, std::vector<TitlePtr>>& de_facto_children,
    ck3::Realm& realm)
{
   std::set<long long> visited;
   std::vector<TitlePtr> to_visit = held_titles;

   while (!to_visit.empty())
   {
      const auto title = to_visit.back();
      to_visit.pop_back();
      if (!title || !visited.insert(title->GetID()).second)
      {
         continue;
      }
      if (title->GetLevel() == ck3::Level::kCounty)
      {
         realm.AddCounty(title);
      }
      const auto children = de_facto_children.find(title->GetID());
      if (children != de_facto_children.end())
      {
         to_visit.insert(to_visit.end(), children->second.begin(), children->second.end());
      }
   }
}
}  // namespace

ck3::Realms::Realms(const Titles& titles, const Characters& characters, const CountyDetails& county_details)
{
   const auto id_title_map = MapTitlesById(titles);
   const auto& all_characters = characters.GetAllCharacters();

   // Independent titles, grouped by the character holding them - a ruler can hold several.
   std::map<long long, std::vector<TitlePtr>> independent_by_holder;
   for (const auto& entry: titles.GetTitles())
   {
      const auto& title = entry.second;
      if (!title->GetHolder().has_value() || !IsIndependent(*title, id_title_map))
      {
         continue;
      }
      const auto holder = all_characters.find(title->GetHolder()->GetID());
      if (holder == all_characters.end() || !HoldsLand(*holder->second))
      {
         continue;
      }
      independent_by_holder[holder->first].emplace_back(title);
   }

   // Reverse index of de facto lieges, so each realm's vassal tree can be walked downwards.
   std::map<long long, std::vector<TitlePtr>> de_facto_children;
   for (const auto& entry: titles.GetTitles())
   {
      const auto& liege = entry.second->GetDeFactoLiege();
      if (liege.has_value())
      {
         de_facto_children[liege->GetID()].emplace_back(entry.second);
      }
   }

   for (const auto& [holder_id, held_titles]: independent_by_holder)
   {
      const auto& holder = all_characters.at(holder_id);
      auto realm = std::make_shared<Realm>(SelectPrimaryTitle(held_titles, *holder), holder);
      for (const auto& title: held_titles)
      {
         realm->AddHeldTitle(title);
      }
      GatherCounties(held_titles, de_facto_children, *realm);

      if (realm->GetPrimaryTitle() && realm->GetPrimaryTitle()->GetCapitalCounty().has_value())
      {
         const auto capital = id_title_map.find(realm->GetPrimaryTitle()->GetCapitalCounty()->GetID());
         if (capital != id_title_map.end())
         {
            realm->SetCapitalCounty(capital->second);
            const auto details = county_details.GetCountyDetails().find(capital->second->GetKey());
            if (details != county_details.GetCountyDetails().end())
            {
               realm->SetCapitalDetails(details->second);
            }
         }
      }
      realms_.emplace_back(std::move(realm));
   }

   std::ranges::sort(realms_, [](const std::shared_ptr<Realm>& lhs, const std::shared_ptr<Realm>& rhs) {
      return lhs->GetCounties().size() > rhs->GetCounties().size();
   });
}

void ck3::Realms::LogRealmReport() const
{
   Log(LogLevel::Info) << "<> " << realms_.size() << " independent realms:";
   for (const auto& realm: realms_)
   {
      const auto& capital = realm->GetCapitalCounty();
      Log(LogLevel::Info) << "   " << realm->GetRealmName() << " (" << realm->GetPrimaryTitle()->GetKey() << ")"
                          << " | ruler: " << realm->GetRulerName()
                          << " | capital: " << (capital ? capital->GetKey() : "none")
                          << " | culture: " << realm->GetCultureName() << " | faith: " << realm->GetFaithName()
                          << " | gov: " << realm->GetGovernment() << " | counties: " << realm->GetCounties().size();
   }
}
