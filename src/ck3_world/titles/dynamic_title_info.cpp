#include "dynamic_title_info.hpp"

#include <iostream>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"

ck3::DynamicTitleInfo::DynamicTitleInfo(std::istream& input_stream)
{
   ParseDynamicTitleInfo(input_stream);
}

void ck3::DynamicTitleInfo::ParseDynamicTitleInfo(std::istream& input_stream)
{
   commonItems::parser titles_parser;
   titles_parser.registerKeyword("key", [this](const std::string&, std::istream& input_stream) {
      dynamic_key_ = commonItems::singleString(input_stream).getString();
   });
   titles_parser.registerKeyword("tier", [this](const std::string&, std::istream& input_stream) {
      dynamic_rank_ = commonItems::singleString(input_stream).getString();
   });
   titles_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   titles_parser.parseStream(input_stream);
   titles_parser.clearRegisteredKeywords();
}
