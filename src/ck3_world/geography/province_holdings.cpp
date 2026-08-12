#include "province_holdings.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "province_holding.hpp"

ck3::ProvinceHoldings::ProvinceHoldings(std::istream& input_stream)
{
   ParseProvinceHoldings(input_stream);
}

void ck3::ProvinceHoldings::ParseProvinceHoldings(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerRegex(R"(\d+)", [this](const std::string& barony_id, std::istream& input_stream) {
      const auto new_barony = std::make_shared<ProvinceHolding>(input_stream);
      province_holdings_.insert(std::make_pair(std::stoi(barony_id), new_barony));
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}
