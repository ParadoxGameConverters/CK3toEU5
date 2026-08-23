#include "county_detail.hpp"

#include <iostream>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "src/ck3_world/cultures/culture.hpp"
#include "src/ck3_world/id_pointer_pair.hpp"
#include "src/ck3_world/religions/faith.hpp"


ck3::CountyDetail::CountyDetail(std::istream& input_stream, std::string county_key): county_key_(std::move(county_key))
{
   ParseCountyDetails(input_stream);
}

void ck3::CountyDetail::ParseCountyDetails(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("development", [this](const std::string&, std::istream& input_stream) {
      development_ = commonItems::singleInt(input_stream).getInt();
   });
   parser.registerKeyword("culture", [this](const std::string&, std::istream& input_stream) {
      culture_ = IdPointerPair<Culture>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerKeyword("faith", [this](const std::string&, std::istream& input_stream) {
      faith_ = IdPointerPair<Faith>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}
