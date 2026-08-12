#include "building.hpp"

#include <iostream>
#include <string>

#include "Parser.h"
#include "ParserHelpers.h"


ck3::Building::Building(std::istream& input_stream)
{
   ParseBuilding(input_stream);
}

void ck3::Building::ParseBuilding(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("type", [this](const std::string&, std::istream& input_stream) {
      type_ = commonItems::singleString(input_stream).getString();
   });
   parser.registerKeyword("disabled", [this](const std::string&, std::istream& input_stream) {
      disabled_ = (commonItems::singleString(input_stream).getString() == "yes");
   });
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}