#ifndef MAPPERS_MAPPERS_H
#define MAPPERS_MAPPERS_H

#include <filesystem>

#include "culture_group_mapper/culture_group_mapper.hpp"
#include "language_mapper/language_mapper.hpp"
#include "province_mapper/province_mapper.hpp"
#include "religion_mapper/religion_mapper.hpp"
#include "tag_mapper/tag_mapper.hpp"

namespace ck3
{
class Cultures;
class LandedTitles;
class Realms;
class Religions;
}  // namespace ck3

namespace mappers
{

// Every CK3 -> EU5 mapping loaded from data/configurables, kept together so the converter has one
// place to ask.
class Mappers
{
  public:
   Mappers() = default;
   explicit Mappers(const std::filesystem::path& configurables_folder);

   [[nodiscard]] const auto& GetTagMapper() const { return tag_mapper_; }
   [[nodiscard]] const auto& GetReligionMapper() const { return religion_mapper_; }
   [[nodiscard]] const auto& GetCultureGroupMapper() const { return culture_group_mapper_; }
   [[nodiscard]] const auto& GetLanguageMapper() const { return language_mapper_; }
   [[nodiscard]] const auto& GetProvinceMapper() const { return province_mapper_; }

   // Reports what in this particular save fails to map, so the gaps can be filled in by hand.
   void LogCoverageReport(const ck3::Realms& realms,
       const ck3::Religions& religions,
       const ck3::Cultures& cultures,
       const ck3::LandedTitles& landed_titles) const;

  private:
   TagMapper tag_mapper_;
   ReligionMapper religion_mapper_;
   CultureGroupMapper culture_group_mapper_;
   LanguageMapper language_mapper_;
   ProvinceMapper province_mapper_;
};

}  // namespace mappers

#endif  // MAPPERS_MAPPERS_H
