#include "flag.hpp"

#include <iostream>
#include <string>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"

ck3::Flag::Flag(std::istream& input_stream)
{
   ParseFlag(input_stream);
}

void ck3::Flag::ParseFlag(std::istream& input_stream)
{
   registerKeyword("flag", [this](const std::string&, std::istream& input_stream) {
      name_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("data", [this](const std::string&, std::istream& input_stream) {
      ParseData(input_stream);
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}

void ck3::Flag::ParseData(std::istream& input_stream)
{
   commonItems::parser data_parser;
   data_parser.registerKeyword("type", [this](const std::string&, std::istream& input_stream) {
      type_ = commonItems::singleString(input_stream).getString();
   });
   data_parser.registerKeyword("identity", [this](const std::string&, std::istream& input_stream) {
      value_ = commonItems::singleString(input_stream).getString();
   });
   data_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   data_parser.parseStream(input_stream);
   data_parser.clearRegisteredKeywords();
}
