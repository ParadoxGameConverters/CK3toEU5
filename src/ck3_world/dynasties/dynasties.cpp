#include "dynasties.hpp"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "dynasty.hpp"
#include "house.hpp"


ck3::Dynasties::Dynasties(std::istream& input_stream)
{
   ParseDynasties(input_stream);
}

void ck3::Dynasties::ParseDynasties(std::istream& input_stream)
{
   registerKeyword("dynasty_house", [this](const std::string&, std::istream& input_stream) {
      ParseHouses(input_stream);
   });
   registerKeyword("dynasties", [this](const std::string&, std::istream& input_stream) {
      ParseOnlyDynasties(input_stream);
   });

   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}

void ck3::Dynasties::ParseOnlyDynasties(std::istream& input_stream)
{
   commonItems::parser dynasties_parser;
   dynasties_parser.registerRegex(R"(\d+)", [this](const std::string& savegame_dynasty_id, std::istream& input_stream) {
      const auto dynasty_blob_as_string = commonItems::stringOfItem(input_stream).getString();
      if (dynasty_blob_as_string.contains('{'))  // check to filter our inactive dynasties
      {
         auto dynasty_stream = std::stringstream(dynasty_blob_as_string);
         const std::shared_ptr<Dynasty> new_dynasty =
             std::make_shared<Dynasty>(dynasty_stream, std::stoll(savegame_dynasty_id));
         dynasties_.insert(std::make_pair(new_dynasty->GetSavegameDynastyID(), new_dynasty));
      }
   });
   dynasties_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   dynasties_parser.parseStream(input_stream);
   dynasties_parser.clearRegisteredKeywords();
}

void ck3::Dynasties::ParseHouses(std::istream& input_stream)
{
   commonItems::parser house_parser;
   house_parser.registerRegex(R"(\d+)", [this](const std::string& house_id, std::istream& input_stream) {
      const auto house_blob_as_string = commonItems::stringOfItem(input_stream).getString();
      if (house_blob_as_string.contains('{'))  // check to filter our inactive houses
      {
         auto house_stream = std::stringstream(house_blob_as_string);
         const std::shared_ptr<House> new_house = std::make_shared<House>(house_stream, std::stoll(house_id));
         houses_.insert(std::make_pair(new_house->GetID(), new_house));
      }
   });
   house_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   house_parser.parseStream(input_stream);
   house_parser.clearRegisteredKeywords();
}
