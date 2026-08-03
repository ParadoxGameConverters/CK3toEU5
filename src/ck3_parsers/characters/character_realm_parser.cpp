#include "character_realm_parser.hpp"

#include <iostream>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"

ck3::CharacterRealmParser::CharacterRealmParser(std::istream& input_stream)
{
   ParseLandedData(input_stream);
}

void ck3::CharacterRealmParser::ParseLandedData(std::istream& input_stream)
{
   registerKeyword("vassal_power_value", [this](const std::string&, std::istream& input_stream) {
      vassal_power_ = commonItems::singleDouble(input_stream).getDouble();
   });
   registerKeyword("government", [this](const std::string&, std::istream& input_stream) {
      government_type_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("laws", [this](const std::string&, std::istream& input_stream) {
      const auto& laws_list = commonItems::stringList(input_stream).getStrings();
      laws_ = std::set(laws_list.begin(), laws_list.end());
   });
   registerKeyword("realm_capital", [this](const std::string&, std::istream& input_stream) {
      realm_capital_ = commonItems::singleLlong(input_stream).getLlong();
   });
   registerKeyword("domain", [this](const std::string&, std::istream& input_stream) {
      for (auto title_id: commonItems::llongList(input_stream).getLlongs())
      {
         domain_.emplace_back(title_id);
      }
   });
   registerKeyword("council", [this](const std::string&, std::istream& input_stream) {
      for (auto character_id: commonItems::llongList(input_stream).getLlongs())
      {
         council_.emplace_back(character_id);
      }
   });
   registerKeyword("royal_court", [this](const std::string&, std::istream& input_stream) {
      ParseCourtData(input_stream);
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

   parseStream(input_stream);
   clearRegisteredKeywords();
}

void ck3::CharacterRealmParser::ParseCourtData(std::istream& input_stream)
{
   commonItems::parser court_parser;
   court_parser.registerKeyword("language", [this](const std::string&, std::istream& input_stream) {
      court_language_ = commonItems::singleString(input_stream).getString();
   });
   court_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   court_parser.parseStream(input_stream);
   court_parser.clearRegisteredKeywords();
}
