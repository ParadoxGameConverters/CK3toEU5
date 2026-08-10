#include "house.hpp"

#include <iostream>

#include "CommonRegexes.h"
#include "ParserHelpers.h"
#include "dynasty.hpp"
#include "src/ck3_world/characters/character.hpp"
#include "src/ck3_world/id_pointer_pair.hpp"

ck3::House::House(std::istream& input_stream, long long house_id): house_id_(house_id)
{
   ParseHouse(input_stream);
}

void ck3::House::ParseHouse(std::istream& input_stream)
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
      dynasty_ = IdPointerPair<Dynasty>(commonItems::singleLlong(input_stream).getLlong());
   });
   registerKeyword("head_of_house", [this](const std::string&, std::istream& input_stream) {
      house_head_ = IdPointerPair<Character>(commonItems::singleLlong(input_stream).getLlong());
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}
