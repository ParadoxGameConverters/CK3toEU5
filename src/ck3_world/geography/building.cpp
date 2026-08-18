#include "building.hpp"

#include <exception>
#include <iostream>
#include <string>

#include "Log.h"
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
      const std::string type = commonItems::singleString(input_stream).getString();
      const auto pos = type.find_last_of('_');
      if (pos != std::string::npos)
      {
         const auto level_string = type.substr(pos + 1, type.size());
         try
         {
            level_ = std::stoi(level_string);
         }
         catch (std::exception&)
         {
            Log(LogLevel::Warning) << "Province building level stoi fail: " << level_string << " from " << type;
            level_ = 1;
         }
         type_ = type.substr(0, pos);
      }
      else
      {
         type_ = type;
      }
   });
   parser.registerKeyword("disabled", [this](const std::string&, std::istream& input_stream) {
      disabled_ = (commonItems::singleString(input_stream).getString() == "yes");
   });
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}