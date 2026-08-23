#include "county_details.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "county_detail.hpp"


ck3::CountyDetails::CountyDetails(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("counties", [this](const std::string&, std::istream& input_stream) {
      ParseCountyDetails(input_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

void ck3::CountyDetails::ParseCountyDetails(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerRegex(R"(c_[A-Za-z0-9_\-\']+)", [this](const std::string& county_key, std::istream& input_stream) {
      auto new_county = std::make_shared<CountyDetail>(input_stream, county_key);
      county_details_.insert(std::make_pair(county_key, new_county));
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}
