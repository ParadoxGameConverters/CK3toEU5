#ifndef MAPPERS_CULTURE_GROUP_MAPPER_H
#define MAPPERS_CULTURE_GROUP_MAPPER_H

#include <filesystem>
#include <istream>
#include <map>
#include <string>
#include <vector>

namespace mappers
{

// Maps CK3 heritages (and languages) onto EU5 culture groups, from
// data/configurables/cultureGroups_map.txt.
//
// This file is REVERSED compared to the other configurables: a link carries a single ck3 heritage
// or language, and one or more eu5 culture groups.
//    link = { eu5 = amazigh_group eu5 = maghrebi_group heritage = heritage_berber }
//
// A link may carry both a heritage and a language, in which case both resolve to the same groups.
class CultureGroupMapper
{
  public:
   CultureGroupMapper() = default;
   explicit CultureGroupMapper(std::istream& input_stream);
   explicit CultureGroupMapper(const std::filesystem::path& file_path);

   // Empty when the heritage, language or name list is unmapped.
   [[nodiscard]] const std::vector<std::string>& GetEU5CultureGroups(const std::string& ck3_heritage) const;
   [[nodiscard]] const std::vector<std::string>& GetEU5CultureGroupsForLanguage(const std::string& ck3_language) const;
   [[nodiscard]] const std::vector<std::string>& GetEU5CultureGroupsForNameList(const std::string& ck3_name_list) const;

   [[nodiscard]] const auto& GetHeritageMappings() const { return heritage_to_eu5_groups_; }
   [[nodiscard]] const auto& GetLanguageMappings() const { return language_to_eu5_groups_; }
   [[nodiscard]] const auto& GetNameListMappings() const { return name_list_to_eu5_groups_; }

  private:
   void ParseMappings(std::istream& input_stream);

   std::map<std::string, std::vector<std::string>> heritage_to_eu5_groups_;
   std::map<std::string, std::vector<std::string>> language_to_eu5_groups_;
   std::map<std::string, std::vector<std::string>> name_list_to_eu5_groups_;
};

}  // namespace mappers

#endif  // MAPPERS_CULTURE_GROUP_MAPPER_H
