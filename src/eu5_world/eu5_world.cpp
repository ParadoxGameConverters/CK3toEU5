#include "eu5_world.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Log.h"
#include "src/ck3_world/characters/characters.hpp"
#include "src/ck3_world/ck3_world.hpp"
#include "src/ck3_world/cultures/culture.hpp"
#include "src/ck3_world/geography/county_detail.hpp"
#include "src/ck3_world/geography/county_details.hpp"
#include "src/ck3_world/realms/realm.hpp"
#include "src/ck3_world/realms/realms.hpp"
#include "src/ck3_world/religions/faith.hpp"
#include "src/ck3_world/titles/landed_title.hpp"
#include "src/ck3_world/titles/landed_titles.hpp"
#include "src/ck3_world/titles/title.hpp"
#include "src/ck3_world/titles/titles.hpp"
#include "src/mappers/mappers.hpp"

namespace
{
using TitlePtr = std::shared_ptr<ck3::Title>;
using IdTitleMap = std::map<long long, TitlePtr>;

IdTitleMap MapTitlesById(const ck3::Titles& titles)
{
   IdTitleMap id_title_map;
   for (const auto& title: titles.GetTitles())
   {
      id_title_map.insert(std::pair(title.second->GetID(), title.second));
   }
   return id_title_map;
}

long long ProvinceOfBarony(const std::string& barony_key, const ck3::LandedTitles& landed_titles)
{
   const auto landed_title = landed_titles.GetLandedTitles().find(barony_key);
   if (landed_title == landed_titles.GetLandedTitles().end())
   {
      return -1;
   }
   return landed_title->second->GetProvince();
}

// Only baronies carry the province ID the province mappings are keyed on. The save lists a county's
// baronies under de_jure_vassals, but leaves the field off entirely for many counties, so fall back
// to the landed_titles hierarchy from the game files, which is always complete.
std::vector<std::string> BaronyKeysOf(const ck3::Title& county,
    const IdTitleMap& id_title_map,
    const ck3::LandedTitles& landed_titles)
{
   std::vector<std::string> barony_keys;
   for (const auto& vassal: county.GetDeJureVassals())
   {
      const auto barony = id_title_map.find(vassal.GetID());
      if (barony != id_title_map.end() && barony->second->GetLevel() == ck3::Level::kBarony)
      {
         barony_keys.emplace_back(barony->second->GetKey());
      }
   }
   if (!barony_keys.empty())
   {
      return barony_keys;
   }
   for (const auto& child_key: landed_titles.GetChildren(county.GetKey()))
   {
      if (child_key.starts_with("b_"))
      {
         barony_keys.emplace_back(child_key);
      }
   }
   return barony_keys;
}

// Most of a CK3 save has no EU5 counterpart - single county tribal realms across Asia and Africa
// that tag_mappings could never sensibly list. Rather than leave their land unowned, they get a
// generated tag built from the title key, so c_dublin becomes DUB where that is free.
std::string GenerateTag(const std::string& title_key, const std::set<std::string>& used_tags)
{
   const auto underscore = title_key.find('_');
   const auto name = underscore == std::string::npos ? title_key : title_key.substr(underscore + 1);

   std::string core;
   for (const char character: name)
   {
      if (std::isalnum(static_cast<unsigned char>(character)) != 0)
      {
         core += static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
      }
   }

   static const std::string kBase36 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   if (core.size() >= 3 && !used_tags.contains(core.substr(0, 3)))
   {
      return core.substr(0, 3);
   }
   // Keep the first two letters recognisable and vary the last.
   if (core.size() >= 2)
   {
      for (const char suffix: kBase36)
      {
         if (const auto candidate = core.substr(0, 2) + suffix; !used_tags.contains(candidate))
         {
            return candidate;
         }
      }
   }
   // Last resort, a purely synthetic tag. EU5 ships none beginning with X0.
   for (const char first: kBase36)
   {
      for (const char second: kBase36)
      {
         if (const auto candidate = std::string("X") + first + second; !used_tags.contains(candidate))
         {
            return candidate;
         }
      }
   }
   return {};
}

// Walks the de facto tree below a ruler's top titles. Counties reached become their land. A duchy
// or higher title held by somebody else stops the walk and is reported back as a vassal, so its
// land goes to the vassal rather than the liege. Counts are deliberately not promoted: EU5 would
// end up with roughly two thousand countries and the map would no longer resemble CK3's.
void GatherLandAndVassals(const std::vector<TitlePtr>& top_titles,
    long long holder_id,
    const std::map<long long, std::vector<TitlePtr>>& de_facto_children,
    std::vector<TitlePtr>& counties,
    std::map<long long, std::vector<TitlePtr>>& vassal_titles_by_holder)
{
   std::set<long long> visited;
   std::set<long long> top_title_ids;
   for (const auto& title: top_titles)
   {
      if (title)
      {
         top_title_ids.insert(title->GetID());
      }
   }

   std::vector<TitlePtr> to_visit = top_titles;
   while (!to_visit.empty())
   {
      const auto title = to_visit.back();
      to_visit.pop_back();
      if (!title || !visited.insert(title->GetID()).second)
      {
         continue;
      }

      const bool is_own_top_title = top_title_ids.contains(title->GetID());
      if (!is_own_top_title && title->GetHolder().has_value() && title->GetHolder()->GetID() != holder_id &&
          title->GetLevel() >= ck3::Level::kDuchy)
      {
         vassal_titles_by_holder[title->GetHolder()->GetID()].emplace_back(title);
         continue;
      }

      if (title->GetLevel() == ck3::Level::kCounty)
      {
         counties.emplace_back(title);
      }
      const auto children = de_facto_children.find(title->GetID());
      if (children != de_facto_children.end())
      {
         to_visit.insert(to_visit.end(), children->second.begin(), children->second.end());
      }
   }
}

// A ruler waiting to become a country. Independent realms seed the list and their vassals are
// appended as the tree is walked, so a vassal's own vassals are handled the same way.
struct PendingRealm
{
   std::shared_ptr<ck3::Realm> realm;
   std::vector<TitlePtr> top_titles;
   long long holder_id = 0;
   std::string liege_tag;  // empty when independent
};

// Vassals have no ck3::Realm of their own - Realms only builds independent ones - so one is put
// together here from the titles they hold under their liege.
std::shared_ptr<ck3::Realm> MakeVassalRealm(const std::vector<TitlePtr>& titles,
    long long holder_id,
    const ck3::CK3World& ck3_world,
    const IdTitleMap& id_title_map)
{
   const auto& characters = ck3_world.GetCharacters().GetAllCharacters();
   const auto holder = characters.find(holder_id);
   if (holder == characters.end() || titles.empty())
   {
      return nullptr;
   }

   // The highest tier title they hold under this liege stands as their primary.
   const auto primary = std::ranges::max_element(titles, {}, [](const TitlePtr& title) {
      return title->GetLevel();
   });

   auto realm = std::make_shared<ck3::Realm>(*primary, holder->second);
   for (const auto& title: titles)
   {
      realm->AddHeldTitle(title);
   }
   if ((*primary)->GetCapitalCounty().has_value())
   {
      const auto capital = id_title_map.find((*primary)->GetCapitalCounty()->GetID());
      if (capital != id_title_map.end())
      {
         realm->SetCapitalCounty(capital->second);
         const auto details = ck3_world.GetCountyDetails().GetCountyDetails().find(capital->second->GetKey());
         if (details != ck3_world.GetCountyDetails().GetCountyDetails().end())
         {
            realm->SetCapitalDetails(details->second);
         }
      }
   }
   return realm;
}

// The save flags the county capital on the barony itself, when it bothers to list the baronies.
std::string CapitalBaronyKey(const ck3::Title& county, const IdTitleMap& id_title_map)
{
   for (const auto& vassal: county.GetDeJureVassals())
   {
      const auto barony = id_title_map.find(vassal.GetID());
      if (barony != id_title_map.end() && barony->second->IsCountyCapital())
      {
         return barony->second->GetKey();
      }
   }
   return {};
}
}  // namespace

namespace
{
// Reverse index of de facto lieges, so a ruler's land can be walked downwards.
std::map<long long, std::vector<TitlePtr>> MapDeFactoChildren(const ck3::Titles& titles)
{
   std::map<long long, std::vector<TitlePtr>> de_facto_children;
   for (const auto& entry: titles.GetTitles())
   {
      if (const auto& liege = entry.second->GetDeFactoLiege(); liege.has_value())
      {
         de_facto_children[liege->GetID()].emplace_back(entry.second);
      }
   }
   return de_facto_children;
}

// Independent realms seed the worklist; their vassals are appended as the land is walked.
std::vector<PendingRealm> SeedPendingRealms(const ck3::Realms& realms)
{
   std::vector<PendingRealm> pending;
   for (const auto& realm: realms.GetRealms())
   {
      if (realm->GetPrimaryTitle() && realm->GetHolder())
      {
         pending.emplace_back(PendingRealm{realm, realm->GetHeldTitles(), realm->GetHolder()->GetID(), {}});
      }
   }
   return pending;
}
}  // namespace

// What the steps of building countries share while they run.
struct eu5::EU5World::Context
{
   const ck3::CK3World& ck3_world;
   const mappers::Mappers& mappers;
   const GameDefinitions& game_definitions;
   const LocationData& location_data;
   IdTitleMap id_title_map;
   std::map<long long, std::vector<TitlePtr>> de_facto_children;
   // An EU5 location can only belong to one country, so first come first served on overlaps.
   std::set<std::string> claimed_locations;
   // Generated tags must avoid every tag EU5 ships as well as everything handed out so far.
   std::set<std::string> used_tags;
   // Tags actually given to a converted country, which is what makes a mapping a duplicate.
   std::set<std::string> assigned_tags;
};

eu5::EU5World::EU5World(const ck3::CK3World& ck3_world,
    const mappers::Mappers& mappers,
    const GameDefinitions& game_definitions,
    const LocationData& location_data):
    conversion_date_(ck3_world.GetConversionDate()),
    culture_resolver_(game_definitions, mappers.GetCultureGroupMapper(), mappers.GetLanguageMapper())
{
   Context context{ck3_world,
       mappers,
       game_definitions,
       location_data,
       MapTitlesById(ck3_world.GetTitles()),
       MapDeFactoChildren(ck3_world.GetTitles()),
       {},
       game_definitions.GetTags(),
       {}};

   // Independent realms first; each one's duchy or higher vassals get appended as they are found,
   // so the list is walked by index while it grows.
   auto pending = SeedPendingRealms(ck3_world.GetRealms());
   for (std::size_t index = 0; index < pending.size(); ++index)
   {
      // A copy, since appending vassals below can reallocate the list.
      const auto current = pending[index];
      const auto& realm = *current.realm;
      if (!realm.GetPrimaryTitle())
      {
         continue;
      }

      // The capital comes first: a capital match beats a title match when picking the tag.
      const auto capital_location = ResolveCapitalLocation(realm, context);
      const auto tag_choice = ChooseTag(realm, capital_location, context);
      if (!tag_choice.has_value())
      {
         continue;
      }

      auto country = std::make_shared<Country>(tag_choice->tag, current.realm);
      if (capital_location.has_value())
      {
         country->SetCapitalLocation(*capital_location);
      }
      MarkForDefinition(*country, *tag_choice, context);
      AssignCapitalFaithAndCulture(*country, realm, capital_location, context);

      // Land is gathered here rather than taken from the realm, because a duchy or higher vassal
      // keeps its own and becomes a country in its own right.
      std::vector<TitlePtr> counties;
      std::map<long long, std::vector<TitlePtr>> vassal_titles_by_holder;
      GatherLandAndVassals(current.top_titles,
          current.holder_id,
          context.de_facto_children,
          counties,
          vassal_titles_by_holder);
      for (const auto& county: counties)
      {
         AddCounty(*country, *county, context);
      }
      AssignPopulationFaithAndCulture(*country, context);

      // Each vassal ruler becomes a country of its own, holding the land under its titles.
      for (const auto& [vassal_holder_id, vassal_titles]: vassal_titles_by_holder)
      {
         if (auto vassal_realm = MakeVassalRealm(vassal_titles, vassal_holder_id, ck3_world, context.id_title_map))
         {
            pending.emplace_back(PendingRealm{vassal_realm, vassal_titles, vassal_holder_id, tag_choice->tag});
         }
      }

      if (!current.liege_tag.empty())
      {
         country->SetLiegeTag(current.liege_tag);
         dependencies_.emplace_back(Dependency{current.liege_tag, tag_choice->tag});
      }
      countries_.emplace_back(std::move(country));
   }

   AssignRanks();
   AssignDevelopment();

   std::ranges::sort(countries_, [](const std::shared_ptr<Country>& lhs, const std::shared_ptr<Country>& rhs) {
      return lhs->GetLocations().size() > rhs->GetLocations().size();
   });
}

std::optional<std::string> eu5::EU5World::ResolveCapitalLocation(const ck3::Realm& realm, const Context& context)
{
   if (!realm.GetCapitalCounty())
   {
      return std::nullopt;
   }
   const auto& landed_titles = context.ck3_world.GetLandedTitles();
   const auto& capital_county = *realm.GetCapitalCounty();
   auto capital_barony_key = CapitalBaronyKey(capital_county, context.id_title_map);
   const auto barony_keys = BaronyKeysOf(capital_county, context.id_title_map, landed_titles);
   // CK3 lists a county's capital barony first, which covers counties the save left bare.
   if (capital_barony_key.empty() && !barony_keys.empty())
   {
      capital_barony_key = barony_keys.front();
   }
   if (capital_barony_key.empty())
   {
      return std::nullopt;
   }
   const auto& locations =
       context.mappers.GetProvinceMapper().GetEU5Locations(ProvinceOfBarony(capital_barony_key, landed_titles));
   if (locations.empty())
   {
      return std::nullopt;
   }
   return locations.front();
}

std::optional<eu5::EU5World::TagChoice> eu5::EU5World::ChooseTag(const ck3::Realm& realm,
    const std::optional<std::string>& capital_location,
    Context& context)
{
   const auto& title_key = realm.GetPrimaryTitle()->GetKey();
   const auto mapped = context.mappers.GetTagMapper().GetEU5Tag(title_key, capital_location);
   // Two realms can map to the same tag - a title and a capital both pointing at it. The second one
   // needs its own, or it would silently take over the first one's land.
   const bool duplicate = mapped.has_value() && context.assigned_tags.contains(*mapped);
   if (mapped.has_value() && !duplicate)
   {
      context.used_tags.insert(*mapped);
      context.assigned_tags.insert(*mapped);
      return TagChoice{*mapped, false};
   }

   auto generated = GenerateTag(title_key, context.used_tags);
   if (generated.empty())
   {
      ++realms_without_tag_;
      return std::nullopt;
   }
   if (duplicate)
   {
      ++duplicate_tags_regenerated_;
   }
   else
   {
      ++realms_with_generated_tag_;
   }
   context.used_tags.insert(generated);
   context.assigned_tags.insert(generated);
   return TagChoice{std::move(generated), true};
}

void eu5::EU5World::MarkForDefinition(Country& country, const TagChoice& tag_choice, const Context& context)
{
   if (tag_choice.generated)
   {
      country.SetNeedsDefinition(true);
      return;
   }
   // tag_mappings still carries EU4 era tags that EU5 never defines - the Ottomans are TUR, not
   // OTT. Writing an undefined tag makes EU5 reject that block, and a rejected block early in the
   // file takes the rest down with it, so those tags need a definition written for them.
   if (context.game_definitions.IsLoaded() && !context.game_definitions.HasTag(tag_choice.tag))
   {
      country.SetNeedsDefinition(true);
      undefined_tags_.insert(tag_choice.tag);
   }
}

void eu5::EU5World::AssignCapitalFaithAndCulture(Country& country,
    const ck3::Realm& realm,
    const std::optional<std::string>& capital_location,
    const Context& context)
{
   const auto& definitions = context.game_definitions;

   // religion_map has the same EU4 era drift - shiite for shia, and religions EU5 simply does not
   // have. Anything EU5 would reject falls back to what it already believes the capital is.
   const auto vanilla_religion =
       capital_location.has_value() ? context.location_data.GetDominantReligion(*capital_location) : std::string{};
   const auto mapped_religion = context.mappers.GetReligionMapper().GetEU5Religion(realm.GetFaithName());
   if (mapped_religion.has_value() && (!definitions.IsLoaded() || definitions.HasReligion(*mapped_religion)))
   {
      country.SetReligion(*mapped_religion);
   }
   else if (!vanilla_religion.empty())
   {
      if (mapped_religion.has_value())
      {
         ++religions_replaced_;
      }
      country.SetReligion(vanilla_religion);
   }

   // A first guess from the capital, so a country whose land holds no pops still has a culture.
   // It is replaced by the population's own once the land is known.
   if (!capital_location.has_value())
   {
      return;
   }
   const auto culture = context.location_data.GetDominantCulture(*capital_location);
   if (!culture.empty() && (!definitions.IsLoaded() || definitions.HasCulture(culture)))
   {
      country.SetCulture(culture);
   }
}

void eu5::EU5World::AddCounty(Country& country, const ck3::Title& county, Context& context)
{
   const auto& landed_titles = context.ck3_world.GetLandedTitles();
   const auto barony_keys = BaronyKeysOf(county, context.id_title_map, landed_titles);
   if (barony_keys.empty())
   {
      // CK3's Chinese noble family counties are flagged landless and hold no barony, so they are
      // expected to contribute nothing rather than being a gap in the conversion.
      const auto landed_title = landed_titles.GetLandedTitles().find(county.GetKey());
      if (landed_title != landed_titles.GetLandedTitles().end() && landed_title->second->IsLandless())
      {
         ++landless_counties_;
      }
      else
      {
         ++counties_without_baronies_;
      }
      return;
   }

   const auto county_data = ReadCountyData(county, context);
   for (const auto& barony_key: barony_keys)
   {
      for (const auto& location:
          context.mappers.GetProvinceMapper().GetEU5Locations(ProvinceOfBarony(barony_key, landed_titles)))
      {
         if (context.claimed_locations.insert(location).second)
         {
            ClaimLocation(country, location, county_data, context);
         }
      }
   }
}

eu5::EU5World::CountyData eu5::EU5World::ReadCountyData(const ck3::Title& county, const Context& context)
{
   CountyData county_data;
   const auto& details = context.ck3_world.GetCountyDetails().GetCountyDetails();
   const auto county_detail = details.find(county.GetKey());
   if (county_detail == details.end())
   {
      return county_data;
   }

   // The county's own faith, not the ruler's, so religious minorities within a realm survive.
   if (const auto faith = county_detail->second->GetFaith().GetPointer().lock(); faith)
   {
      const auto mapped = context.mappers.GetReligionMapper().GetEU5Religion(faith->GetTag());
      const auto& definitions = context.game_definitions;
      if (mapped.has_value() && (!definitions.IsLoaded() || definitions.HasReligion(*mapped)))
      {
         county_data.religion = *mapped;
      }
   }
   // CK3's own development for this county, carried so the player's building up of their realm is
   // not thrown away in favour of EU5's default terrain score.
   county_data.development = county_detail->second->GetDevelopment();
   // The county's own culture, kept as the CK3 object so each location can be judged against
   // whatever EU5 already has there.
   county_data.culture = county_detail->second->GetCulture().GetPointer().lock();
   return county_data;
}

void eu5::EU5World::ClaimLocation(Country& country,
    const std::string& location,
    const CountyData& county_data,
    const Context& context)
{
   country.AddLocation(location);
   if (!county_data.religion.empty())
   {
      location_religions_.insert_or_assign(location, county_data.religion);
   }
   if (county_data.development >= 0)
   {
      location_development_.insert_or_assign(location, county_data.development);
   }

   // EU5 is far more granular than CK3, so its own culture is kept wherever it is a plausible
   // member of the CK3 one. Only where the two genuinely disagree - because a campaign moved a
   // culture somewhere it does not belong - is the location rewritten.
   if (!county_data.culture)
   {
      return;
   }
   const auto vanilla_culture = context.location_data.GetDominantCulture(location);
   if (vanilla_culture.empty() || culture_resolver_.IsCompatible(*county_data.culture, vanilla_culture))
   {
      return;
   }
   const auto converted_culture = culture_resolver_.Resolve(*county_data.culture);
   if (converted_culture.empty())
   {
      return;
   }
   location_cultures_.insert_or_assign(location, converted_culture);
   culture_resolver_.MarkUsed(converted_culture);
   ++cultures_replaced_;
}

eu5::EU5World::PopulationWeights eu5::EU5World::WeighPopulation(const Country& country, const Context& context) const
{
   PopulationWeights weights;
   const auto& location_data = context.location_data;
   for (const auto& location: country.GetLocations())
   {
      const auto pops = location_data.GetPops().find(location);
      if (pops == location_data.GetPops().end())
      {
         continue;
      }
      // Majority pops get rewritten to the county's converted culture and faith, so weigh what the
      // game will actually see rather than what EU5 shipped.
      const auto converted_culture = location_cultures_.find(location);
      const auto vanilla_culture = location_data.GetDominantCulture(location);
      const auto converted_religion = location_religions_.find(location);
      const auto vanilla_religion = location_data.GetDominantReligion(location);
      for (const auto& pop: pops->second)
      {
         const bool culture_rewritten = converted_culture != location_cultures_.end() && pop.culture == vanilla_culture;
         weights.culture[culture_rewritten ? converted_culture->second : pop.culture] += pop.size;
         const bool religion_rewritten =
             converted_religion != location_religions_.end() && pop.religion == vanilla_religion;
         weights.religion[religion_rewritten ? converted_religion->second : pop.religion] += pop.size;
      }
   }
   return weights;
}

void eu5::EU5World::AssignPopulationFaithAndCulture(Country& country, const Context& context)
{
   // EU5 checks a country's religion and culture against its own population and complains for
   // every one that disagrees. The capital alone is a poor proxy, so now that the land is known both
   // are taken from the pops the country actually holds, weighted by size.
   const auto weights = WeighPopulation(country, context);
   const auto heaviest = [](const std::map<std::string, double>& weight) {
      return std::ranges::max_element(weight, {}, [](const auto& entry) {
         return entry.second;
      })->first;
   };
   const auto& definitions = context.game_definitions;

   if (!weights.culture.empty())
   {
      const auto culture = heaviest(weights.culture);
      // A generated culture is not in EU5's own definitions but will be shipped alongside.
      if (!definitions.IsLoaded() || definitions.HasCulture(culture) ||
          culture_resolver_.GetGeneratedCultures().contains(culture))
      {
         country.SetCulture(culture);
         culture_resolver_.MarkUsed(culture);
      }
   }
   if (!weights.religion.empty())
   {
      const auto religion = heaviest(weights.religion);
      if (!definitions.IsLoaded() || definitions.HasReligion(religion))
      {
         country.SetReligion(religion);
      }
   }
}

void eu5::EU5World::LogReport() const
{
   LogLandReport();
   LogTagReport();
   LogFaithAndCultureReport();
   LogCountryList();
}

void eu5::EU5World::LogLandReport() const
{
   std::size_t total_locations = 0;
   std::size_t without_capital = 0;
   std::size_t without_religion = 0;
   for (const auto& country: countries_)
   {
      total_locations += country->GetLocations().size();
      without_capital += country->GetCapitalLocation().has_value() ? 0 : 1;
      without_religion += country->GetReligion().has_value() ? 0 : 1;
   }

   Log(LogLevel::Info) << "<> " << countries_.size() << " EU5 countries holding " << total_locations << " locations.";
   if (realms_without_tag_ > 0)
   {
      Log(LogLevel::Warning) << "   " << realms_without_tag_ << " realms could not be given any tag and were dropped.";
   }
   if (without_capital > 0)
   {
      Log(LogLevel::Warning) << "   " << without_capital << " countries have no EU5 capital location.";
   }
   if (without_religion > 0)
   {
      Log(LogLevel::Warning) << "   " << without_religion << " countries have no EU5 religion.";
   }
   if (counties_without_baronies_ > 0)
   {
      Log(LogLevel::Warning) << "   " << counties_without_baronies_ << " counties had no baronies to draw land from.";
   }
}

void eu5::EU5World::LogTagReport() const
{
   if (realms_with_generated_tag_ > 0)
   {
      Log(LogLevel::Info) << "   " << realms_with_generated_tag_
                          << " realms had no mapping and were given a generated tag.";
   }
   if (duplicate_tags_regenerated_ > 0)
   {
      Log(LogLevel::Warning) << "   " << duplicate_tags_regenerated_
                             << " realms mapped to a tag already taken and were given a generated one.";
   }
   if (!undefined_tags_.empty())
   {
      std::string tag_list;
      for (const auto& tag: undefined_tags_)
      {
         tag_list += tag + " ";
      }
      Log(LogLevel::Info) << "   " << undefined_tags_.size()
                          << " tags are not defined by EU5 and get a generated definition: " << tag_list;
   }
}

void eu5::EU5World::LogFaithAndCultureReport() const
{
   if (religions_replaced_ > 0)
   {
      Log(LogLevel::Warning) << "   " << religions_replaced_
                             << " countries had a mapped religion EU5 does not define, replaced with the capital's.";
   }
   Log(LogLevel::Info) << "   " << cultures_replaced_
                       << " locations had a culture CK3 disagrees with, rewritten from the county.";
   const auto generated_cultures = culture_resolver_.GetUsedGeneratedCultures();
   if (!generated_cultures.empty())
   {
      Log(LogLevel::Info) << "   " << generated_cultures.size()
                          << " CK3 cultures have no EU5 equivalent and get a generated definition.";
   }
   if (!culture_resolver_.GetUnresolved().empty())
   {
      Log(LogLevel::Warning) << "   " << culture_resolver_.GetUnresolved().size()
                             << " CK3 cultures map to no EU5 culture group; EU5's own culture stands there.";
   }
}

void eu5::EU5World::LogCountryList() const
{
   for (const auto& country: countries_)
   {
      if (country->GetLocations().empty())
      {
         continue;
      }
      Log(LogLevel::Info) << "   " << country->GetTag() << " <- " << country->GetSourceRealm()->GetRealmName() << " ("
                          << country->GetSourceRealm()->GetPrimaryTitle()->GetKey() << ")"
                          << " | capital: " << country->GetCapitalLocation().value_or("none")
                          << " | religion: " << country->GetReligion().value_or("none")
                          << " | locations: " << country->GetLocations().size();
   }
}

namespace
{
// EU5 ranks, weakest first. A country's rank comes from its CK3 title tier.
const std::vector<std::string> kRanks = {"rank_county", "rank_duchy", "rank_kingdom", "rank_empire"};

std::size_t RankIndexFor(ck3::Level tier)
{
   switch (tier)
   {
      case ck3::Level::kEmpire:
      case ck3::Level::kHegemony:
         return 3;
      case ck3::Level::kKingdom:
         return 2;
      case ck3::Level::kDuchy:
         return 1;
      default:
         return 0;
   }
}
}  // namespace

void eu5::EU5World::AssignRanks()
{
   std::map<std::string, std::size_t> rank_index;
   for (const auto& country: countries_)
   {
      rank_index[country->GetTag()] = RankIndexFor(country->GetSourceRealm()->GetTier());
   }

   // EU5 refuses a vassal that outranks its liege - vassal.txt requires
   // country_rank_level >= scope:target.country_rank_level - and silently drops the relationship.
   // Walking lieges first is not enough, because a liege can itself be someone's vassal, so this
   // repeats until nothing more needs lowering.
   bool changed = true;
   while (changed)
   {
      changed = false;
      for (const auto& country: countries_)
      {
         const auto& liege_tag = country->GetLiegeTag();
         if (liege_tag.empty())
         {
            continue;
         }
         const auto liege = rank_index.find(liege_tag);
         if (liege == rank_index.end())
         {
            continue;
         }
         auto& own = rank_index[country->GetTag()];
         if (own > liege->second)
         {
            own = liege->second;
            ++vassals_demoted_;
            changed = true;
         }
      }
   }

   for (const auto& country: countries_)
   {
      country->SetRank(kRanks[rank_index[country->GetTag()]]);
   }
}

void eu5::EU5World::AssignDevelopment()
{
   if (location_development_.empty())
   {
      return;
   }

   // EU5 only hands a development bonus to a few hundred notable places; giving one to every
   // converted location would inflate the whole world. The save's own median is the baseline, so
   // only land the player developed beyond ordinary gets a bonus, and the scale calibrates itself
   // whether the save is an early start or a late one.
   std::vector<int> values;
   values.reserve(location_development_.size());
   for (const auto& [location, development]: location_development_)
   {
      values.emplace_back(development);
   }
   std::ranges::sort(values);
   development_baseline_ = values[values.size() / 2];

   // EU5's own bonuses top out at 30.
   constexpr int kMaxBonus = 30;
   for (const auto& [location, development]: location_development_)
   {
      const auto bonus = std::min(development - development_baseline_, kMaxBonus);
      if (bonus > 0)
      {
         development_bonuses_.insert_or_assign(location, bonus);
      }
   }

   // Technology is judged on absolute CK3 development, not against the save's median. Half of any
   // save sits below its own median by definition, so a relative test would demote half the world
   // no matter when the save was taken. Absolute thresholds instead mean an early save converts as
   // genuinely less advanced and a late one as more, which is the point of converting at 1337.
   constexpr double kAdvanced = 15.0;
   constexpr double kModerate = 8.0;
   constexpr double kRudimentary = 3.0;
   for (const auto& country: countries_)
   {
      double total = 0;
      int counted = 0;
      for (const auto& location: country->GetLocations())
      {
         const auto development = location_development_.find(location);
         if (development != location_development_.end())
         {
            total += development->second;
            ++counted;
         }
      }
      if (counted == 0)
      {
         continue;
      }
      const auto mean = total / counted;
      if (mean >= kAdvanced)
      {
         country->SetTechnologyLevel(3);
      }
      else if (mean >= kModerate)
      {
         country->SetTechnologyLevel(2);
      }
      else if (mean >= kRudimentary)
      {
         country->SetTechnologyLevel(1);
      }
      else
      {
         country->SetTechnologyLevel(0);
      }
   }
}
