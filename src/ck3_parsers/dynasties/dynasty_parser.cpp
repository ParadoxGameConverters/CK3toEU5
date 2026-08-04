#include "dynasty_parser.hpp"

#include <iostream>

#include "CommonRegexes.h"
#include "ParserHelpers.h"

ck3::DynastyParser::DynastyParser(std::istream& input_stream, long long savegame_dynasty_id):
    savegame_dynasty_id_(savegame_dynasty_id)
{
   ParseDynasty(input_stream);
}

void ck3::DynastyParser::ParseDynasty(std::istream& input_stream)
{
   registerKeyword("key", [this](const std::string&, std::istream& input_stream) {
      dynasty_id_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("good_for_realm_name", [this](const std::string&, std::istream& input_stream) {
      appropriate_realm_name_ = commonItems::singleString(input_stream).getString() == "yes";
   });
   // registerKeyword("coat_of_arms_id", [this](const std::string&, std::istream& input_stream) {
   //    coa = std::make_pair(commonItems::singleLlong(input_stream).getLlong(), nullptr);
   // });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}
