#include "religion_mapper.hpp"

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
// One link: a single eu5 religion, and every ck3 faith that should become it.
struct ReligionLink
{
   std::optional<std::string> eu5_religion;
   std::vector<std::string> ck3_faiths;
};

ReligionLink ParseLink(std::istream& input_stream)
{
   ReligionLink link;
   commonItems::parser link_parser;
   link_parser.registerKeyword("eu5", [&link](std::istream& input_stream) {
      link.eu5_religion = commonItems::getString(input_stream);
   });
   link_parser.registerKeyword("ck3", [&link](std::istream& input_stream) {
      link.ck3_faiths.emplace_back(commonItems::getString(input_stream));
   });
   link_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   link_parser.parseStream(input_stream);
   link_parser.clearRegisteredKeywords();
   return link;
}
}  // namespace

mappers::ReligionMapper::ReligionMapper(std::istream& input_stream)
{
   ParseMappings(input_stream);
}

mappers::ReligionMapper::ReligionMapper(const std::filesystem::path& file_path)
{
   std::ifstream file_stream(file_path);
   if (!file_stream.is_open())
   {
      throw std::runtime_error("Could not open " + file_path.string() + " for religion mappings!");
   }
   ParseMappings(file_stream);
   Log(LogLevel::Info) << "<> Loaded " << ck3_faith_to_eu5_religion_.size() << " religion mappings.";
}

void mappers::ReligionMapper::ParseMappings(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("link", [this](std::istream& input_stream) {
      const auto link = ParseLink(input_stream);
      if (!link.eu5_religion.has_value() || *link.eu5_religion == "???")
      {
         return;
      }
      for (const auto& ck3_faith: link.ck3_faiths)
      {
         ck3_faith_to_eu5_religion_.insert_or_assign(ck3_faith, *link.eu5_religion);
      }
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

std::optional<std::string> mappers::ReligionMapper::GetEU5Religion(const std::string& ck3_faith) const
{
   const auto mapping = ck3_faith_to_eu5_religion_.find(ck3_faith);
   if (mapping == ck3_faith_to_eu5_religion_.end())
   {
      return std::nullopt;
   }
   return mapping->second;
}
