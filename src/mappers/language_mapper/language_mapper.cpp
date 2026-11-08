#include "language_mapper.hpp"

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
struct LanguageLink
{
   std::optional<std::string> eu5_language;
   std::vector<std::string> ck3_languages;
   std::vector<std::string> ck3_name_lists;
};

LanguageLink ParseLink(std::istream& input_stream)
{
   LanguageLink link;
   commonItems::parser link_parser;
   link_parser.registerKeyword("eu5", [&link](std::istream& input_stream) {
      link.eu5_language = commonItems::getString(input_stream);
   });
   link_parser.registerKeyword("ck3", [&link](std::istream& input_stream) {
      link.ck3_languages.emplace_back(commonItems::getString(input_stream));
   });
   link_parser.registerKeyword("name_list", [&link](std::istream& input_stream) {
      link.ck3_name_lists.emplace_back(commonItems::getString(input_stream));
   });
   link_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   link_parser.parseStream(input_stream);
   link_parser.clearRegisteredKeywords();
   return link;
}
}  // namespace

mappers::LanguageMapper::LanguageMapper(std::istream& input_stream)
{
   ParseMappings(input_stream);
}

mappers::LanguageMapper::LanguageMapper(const std::filesystem::path& file_path)
{
   std::ifstream file_stream(file_path);
   if (!file_stream.is_open())
   {
      throw std::runtime_error("Could not open " + file_path.string() + " for language mappings!");
   }
   ParseMappings(file_stream);
   Log(LogLevel::Info) << "<> Loaded " << ck3_language_to_eu5_language_.size() << " language mappings and "
                       << ck3_name_list_to_eu5_language_.size() << " name list mappings.";
}

void mappers::LanguageMapper::ParseMappings(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("link", [this](std::istream& input_stream) {
      const auto link = ParseLink(input_stream);
      if (!link.eu5_language.has_value() || *link.eu5_language == "???")
      {
         return;
      }
      for (const auto& ck3_language: link.ck3_languages)
      {
         ck3_language_to_eu5_language_.insert_or_assign(ck3_language, *link.eu5_language);
      }
      for (const auto& ck3_name_list: link.ck3_name_lists)
      {
         ck3_name_list_to_eu5_language_.insert_or_assign(ck3_name_list, *link.eu5_language);
      }
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

std::optional<std::string> mappers::LanguageMapper::GetEU5Language(const std::string& ck3_language) const
{
   const auto mapping = ck3_language_to_eu5_language_.find(ck3_language);
   if (mapping == ck3_language_to_eu5_language_.end())
   {
      return std::nullopt;
   }
   return mapping->second;
}

std::optional<std::string> mappers::LanguageMapper::GetEU5LanguageForNameList(const std::string& ck3_name_list) const
{
   const auto mapping = ck3_name_list_to_eu5_language_.find(ck3_name_list);
   if (mapping == ck3_name_list_to_eu5_language_.end())
   {
      return std::nullopt;
   }
   return mapping->second;
}
