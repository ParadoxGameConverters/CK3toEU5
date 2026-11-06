#include "tag_mapper.hpp"

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"

namespace
{
struct TagLink
{
   std::optional<std::string> eu5_tag;
   std::optional<std::string> ck3_title;
   std::vector<std::string> capitals;
};

TagLink ParseLink(std::istream& input_stream)
{
   TagLink link;
   commonItems::parser link_parser;
   link_parser.registerKeyword("eu5", [&link](std::istream& input_stream) {
      link.eu5_tag = commonItems::getString(input_stream);
   });
   link_parser.registerKeyword("ck3", [&link](std::istream& input_stream) {
      link.ck3_title = commonItems::getString(input_stream);
   });
   link_parser.registerKeyword("capitals", [&link](std::istream& input_stream) {
      for (const auto& capital: commonItems::getStrings(input_stream))
      {
         link.capitals.emplace_back(capital);
      }
   });
   link_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   link_parser.parseStream(input_stream);
   link_parser.clearRegisteredKeywords();
   return link;
}
}  // namespace

mappers::TagMapper::TagMapper(std::istream& input_stream)
{
   ParseMappings(input_stream);
}

mappers::TagMapper::TagMapper(const std::filesystem::path& file_path)
{
   std::ifstream file_stream(file_path);
   if (!file_stream.is_open())
   {
      throw std::runtime_error("Could not open " + file_path.string() + " for tag mappings!");
   }
   ParseMappings(file_stream);
   Log(LogLevel::Info) << "<> Loaded " << title_to_tag_.size() << " title tag mappings and " << capital_to_tag_.size()
                       << " capital tag mappings.";
}

void mappers::TagMapper::ParseMappings(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("link", [this](std::istream& input_stream) {
      const auto link = ParseLink(input_stream);
      if (!link.eu5_tag.has_value() || *link.eu5_tag == "???")
      {
         return;
      }
      if (link.ck3_title.has_value())
      {
         title_to_tag_.insert_or_assign(*link.ck3_title, *link.eu5_tag);
      }
      for (const auto& capital: link.capitals)
      {
         capital_to_tag_.insert_or_assign(capital, *link.eu5_tag);
      }
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

std::optional<std::string> mappers::TagMapper::GetEU5Tag(const std::string& ck3_title_key,
    const std::optional<std::string>& capital_location) const
{
   // Having a capital defined takes precedence over the title.
   if (capital_location.has_value())
   {
      const auto by_capital = capital_to_tag_.find(*capital_location);
      if (by_capital != capital_to_tag_.end())
      {
         return by_capital->second;
      }
   }
   const auto by_title = title_to_tag_.find(ck3_title_key);
   if (by_title == title_to_tag_.end())
   {
      return std::nullopt;
   }
   return by_title->second;
}
