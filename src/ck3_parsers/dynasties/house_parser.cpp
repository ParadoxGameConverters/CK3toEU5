#include "house_parser.hpp"

#include <iostream>

#include "CommonRegexes.h"
#include "ParserHelpers.h"

ck3::HouseParser::HouseParser(std::istream& input_stream, long long house_id): house_id_(house_id)
{
   ParseHouse(input_stream);
}

void ck3::HouseParser::ParseHouse(std::istream& input_stream)
{
   registerKeyword("key", [this](std::istream& input_stream) {
      key_ = commonItems::getString(input_stream);
   });
   registerKeyword("name", [this](const std::string&, std::istream& input_stream) {
      name_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("localized_name", [this](const std::string&, std::istream& input_stream) {
      localized_name_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("prefix", [this](const std::string&, std::istream& input_stream) {
      prefix_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("dynasty", [this](const std::string&, std::istream& input_stream) {
      dynasty_ = commonItems::singleLlong(input_stream).getLlong();
   });
   registerKeyword("head_of_house", [this](const std::string&, std::istream& input_stream) {
      house_head_ = commonItems::singleLlong(input_stream).getLlong();
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}
