#include "cultures.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "culture.hpp"

ck3::Cultures::Cultures(std::istream& input_stream)
{
   ParseCultureManager(input_stream);
}

void ck3::Cultures::ParseCultureManager(std::istream& input_stream)
{
   registerKeyword("cultures", [this](std::istream& input_stream) {
      ParseCultures(input_stream);
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}

void ck3::Cultures::ParseCultures(std::istream& input_stream)
{
   commonItems::parser cultures_parser;
   cultures_parser.registerRegex(R"(\d+)", [this](const std::string& culture_id, std::istream& input_stream) {
      const auto new_culture = std::make_shared<Culture>(input_stream, std::stoll(culture_id));
      cultures_.insert(std::make_pair(new_culture->GetID(), new_culture));
   });
   cultures_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   cultures_parser.parseStream(input_stream);
   cultures_parser.clearRegisteredKeywords();
}
