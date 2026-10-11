#include "culture_group_mapper.hpp"

#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"

namespace
{
const std::vector<std::string> kNoGroups;

struct CultureGroupLink
{
   std::vector<std::string> eu5_groups;
   std::optional<std::string> ck3_heritage;
   std::optional<std::string> ck3_language;
   std::optional<std::string> ck3_name_list;
};

CultureGroupLink ParseLink(std::istream& input_stream)
{
   CultureGroupLink link;
   commonItems::parser link_parser;
   link_parser.registerKeyword("eu5", [&link](std::istream& input_stream) {
      link.eu5_groups.emplace_back(commonItems::getString(input_stream));
   });
   link_parser.registerKeyword("heritage", [&link](std::istream& input_stream) {
      link.ck3_heritage = commonItems::getString(input_stream);
   });
   link_parser.registerKeyword("language", [&link](std::istream& input_stream) {
      link.ck3_language = commonItems::getString(input_stream);
   });
   link_parser.registerKeyword("name_list", [&link](std::istream& input_stream) {
      link.ck3_name_list = commonItems::getString(input_stream);
   });
   link_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   link_parser.parseStream(input_stream);
   link_parser.clearRegisteredKeywords();
   return link;
}

// The shipped file marks unresolved entries with a ??? placeholder rather than a real group.
std::vector<std::string> WithoutPlaceholders(const std::vector<std::string>& groups)
{
   std::vector<std::string> resolved;
   for (const auto& group: groups)
   {
      if (group != "???")
      {
         resolved.emplace_back(group);
      }
   }
   return resolved;
}
}  // namespace

mappers::CultureGroupMapper::CultureGroupMapper(std::istream& input_stream)
{
   ParseMappings(input_stream);
}

mappers::CultureGroupMapper::CultureGroupMapper(const std::filesystem::path& file_path)
{
   std::ifstream file_stream(file_path);
   if (!file_stream.is_open())
   {
      throw std::runtime_error("Could not open " + file_path.string() + " for culture group mappings!");
   }
   ParseMappings(file_stream);
   Log(LogLevel::Info) << "<> Loaded " << heritage_to_eu5_groups_.size() << " heritage mappings and "
                       << language_to_eu5_groups_.size() << " language culture group mappings.";
}

void mappers::CultureGroupMapper::ParseMappings(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("link", [this](std::istream& input_stream) {
      const auto link = ParseLink(input_stream);
      const auto groups = WithoutPlaceholders(link.eu5_groups);
      if (groups.empty())
      {
         return;
      }
      if (link.ck3_heritage.has_value())
      {
         heritage_to_eu5_groups_.insert_or_assign(*link.ck3_heritage, groups);
      }
      if (link.ck3_language.has_value())
      {
         language_to_eu5_groups_.insert_or_assign(*link.ck3_language, groups);
      }
      if (link.ck3_name_list.has_value())
      {
         name_list_to_eu5_groups_.insert_or_assign(*link.ck3_name_list, groups);
      }
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

const std::vector<std::string>& mappers::CultureGroupMapper::GetEU5CultureGroups(const std::string& ck3_heritage) const
{
   const auto mapping = heritage_to_eu5_groups_.find(ck3_heritage);
   return mapping == heritage_to_eu5_groups_.end() ? kNoGroups : mapping->second;
}

const std::vector<std::string>& mappers::CultureGroupMapper::GetEU5CultureGroupsForLanguage(
    const std::string& ck3_language) const
{
   const auto mapping = language_to_eu5_groups_.find(ck3_language);
   return mapping == language_to_eu5_groups_.end() ? kNoGroups : mapping->second;
}

const std::vector<std::string>& mappers::CultureGroupMapper::GetEU5CultureGroupsForNameList(
    const std::string& ck3_name_list) const
{
   const auto mapping = name_list_to_eu5_groups_.find(ck3_name_list);
   return mapping == name_list_to_eu5_groups_.end() ? kNoGroups : mapping->second;
}
