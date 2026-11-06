#include "mappers.hpp"

#include <set>
#include <string>

#include "Log.h"
#include "src/ck3_world/cultures/culture.hpp"
#include "src/ck3_world/cultures/cultures.hpp"
#include "src/ck3_world/realms/realms.hpp"
#include "src/ck3_world/religions/faith.hpp"
#include "src/ck3_world/religions/religions.hpp"
#include "src/ck3_world/titles/landed_titles.hpp"
#include "src/ck3_world/titles/title.hpp"

namespace
{
// Lists what failed to map without drowning the log on a big save.
constexpr size_t kMaxListed = 25;

// Both sets hold distinct keys, so the counts are comparable.
void LogMissing(const std::string& what, const std::set<std::string>& seen, const std::set<std::string>& missing)
{
   const auto mapped = seen.size() - missing.size();
   if (missing.empty())
   {
      Log(LogLevel::Info) << "   " << what << ": " << mapped << "/" << seen.size() << " mapped.";
      return;
   }

   Log(LogLevel::Warning) << "   " << what << ": " << mapped << "/" << seen.size() << " mapped, " << missing.size()
                          << " unmapped:";
   size_t listed = 0;
   for (const auto& entry: missing)
   {
      if (listed == kMaxListed)
      {
         Log(LogLevel::Warning) << "      ... and " << missing.size() - kMaxListed << " more.";
         break;
      }
      Log(LogLevel::Warning) << "      " << entry;
      ++listed;
   }
}
}  // namespace

mappers::Mappers::Mappers(const std::filesystem::path& configurables_folder):
    tag_mapper_(configurables_folder / "tag_mappings.txt"),
    religion_mapper_(configurables_folder / "religion_map.txt"),
    culture_group_mapper_(configurables_folder / "cultureGroups_map.txt"),
    language_mapper_(configurables_folder / "language_map.txt"),
    province_mapper_(configurables_folder / "province_mappings.txt")
{
}

void mappers::Mappers::LogCoverageReport(const ck3::Realms& realms,
    const ck3::Religions& religions,
    const ck3::Cultures& cultures,
    const ck3::LandedTitles& landed_titles) const
{
   Log(LogLevel::Info) << "-> Mapping coverage for this save:";

   // Realms -> EU5 tags. Without province mappings this can only match on the title key; once
   // capitals resolve to EU5 locations, capital matches will take precedence and close some gaps.
   std::set<std::string> realm_titles;
   std::set<std::string> unmapped_realms;
   for (const auto& realm: realms.GetRealms())
   {
      if (!realm->GetPrimaryTitle())
      {
         continue;
      }
      const auto& title_key = realm->GetPrimaryTitle()->GetKey();
      realm_titles.insert(title_key);
      if (!tag_mapper_.GetEU5Tag(title_key).has_value())
      {
         unmapped_realms.insert(title_key);
      }
   }
   LogMissing("realm titles -> tags", realm_titles, unmapped_realms);

   std::set<std::string> faith_tags;
   std::set<std::string> unmapped_faiths;
   for (const auto& faith: religions.GetFaiths())
   {
      const auto& tag = faith.second->GetTag();
      if (tag.empty())
      {
         continue;
      }
      faith_tags.insert(tag);
      if (!religion_mapper_.GetEU5Religion(tag).has_value())
      {
         unmapped_faiths.insert(tag);
      }
   }
   LogMissing("faiths -> religions", faith_tags, unmapped_faiths);

   std::set<std::string> heritages;
   std::set<std::string> unmapped_heritages;
   std::set<std::string> languages;
   std::set<std::string> unmapped_languages;
   for (const auto& culture: cultures.GetCultures())
   {
      const auto& heritage = culture.second->GetHeritage();
      if (!heritage.empty())
      {
         heritages.insert(heritage);
         if (culture_group_mapper_.GetEU5CultureGroups(heritage).empty())
         {
            unmapped_heritages.insert(heritage);
         }
      }
      const auto& language = culture.second->GetLanguage();
      if (!language.empty())
      {
         languages.insert(language);
         if (!language_mapper_.GetEU5Language(language).has_value())
         {
            unmapped_languages.insert(language);
         }
      }
   }
   LogMissing("heritages -> culture groups", heritages, unmapped_heritages);
   LogMissing("languages -> languages", languages, unmapped_languages);

   // Every CK3 barony carries the province ID the province mappings are keyed on.
   std::set<std::string> baronies;
   std::set<std::string> unmapped_baronies;
   for (const auto& landed_title: landed_titles.GetLandedTitles())
   {
      const auto province = landed_title.second->GetProvince();
      if (province < 0)
      {
         continue;
      }
      baronies.insert(landed_title.first);
      if (province_mapper_.GetEU5Locations(province).empty())
      {
         unmapped_baronies.insert(landed_title.first);
      }
   }
   LogMissing("CK3 baronies -> EU5 locations", baronies, unmapped_baronies);
}
