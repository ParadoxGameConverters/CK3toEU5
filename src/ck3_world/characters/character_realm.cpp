#include "character_realm.hpp"

#include <iostream>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "src/ck3_world/id_pointer_pair.hpp"


ck3::CharacterRealm::CharacterRealm(std::istream& input_stream)
{
   ParseLandedData(input_stream);
}

void ck3::CharacterRealm::ParseLandedData(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("vassal_power_value", [this](const std::string&, std::istream& input_stream) {
      vassal_power_ = commonItems::singleDouble(input_stream).getDouble();
   });
   parser.registerKeyword("government", [this](const std::string&, std::istream& input_stream) {
      government_type_ = commonItems::singleString(input_stream).getString();
   });
   parser.registerKeyword("laws", [this](const std::string&, std::istream& input_stream) {
      const auto& laws_list = commonItems::stringList(input_stream).getStrings();
      laws_ = std::set(laws_list.begin(), laws_list.end());
   });
   parser.registerKeyword("realm_capital", [this](const std::string&, std::istream& input_stream) {
      realm_capital_ = IdPointerPair<Title>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerKeyword("domain", [this](const std::string&, std::istream& input_stream) {
      for (auto title_id: commonItems::llongList(input_stream).getLlongs())
      {
         domain_.emplace_back(title_id);
      }
   });
   parser.registerKeyword("council", [this](const std::string&, std::istream& input_stream) {
      for (auto character_id: commonItems::llongList(input_stream).getLlongs())
      {
         council_.emplace_back(character_id);
      }
   });
   parser.registerKeyword("royal_court", [this](const std::string&, std::istream& input_stream) {
      ParseCourtData(input_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

void ck3::CharacterRealm::ParseCourtData(std::istream& input_stream)
{
   commonItems::parser court_parser;
   court_parser.registerKeyword("language", [this](const std::string&, std::istream& input_stream) {
      court_language_ = commonItems::singleString(input_stream).getString();
   });
   court_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   court_parser.parseStream(input_stream);
   court_parser.clearRegisteredKeywords();
}
