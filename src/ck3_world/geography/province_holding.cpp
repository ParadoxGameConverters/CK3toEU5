#include "province_holding.hpp"

#include <iostream>
#include <sstream>
#include <string>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "building.hpp"

ck3::ProvinceHolding::ProvinceHolding(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("holding", [this](const std::string&, std::istream& input_stream) {
      ParseHolding(input_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

void ck3::ProvinceHolding::ParseHolding(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("type", [this](const std::string&, std::istream& input_stream) {
      holding_type_ = commonItems::singleString(input_stream).getString();
   });
   parser.registerKeyword("special_building_type", [this](const std::string&, std::istream& input_stream) {
      special_building_ = commonItems::singleString(input_stream).getString();
   });
   parser.registerKeyword("duchy_capital_building", [this](const std::string&, std::istream& input_stream) {
      duchy_capital_building_ = Building(input_stream);
   });
   parser.registerKeyword("buildings", [this](const std::string&, std::istream& input_stream) {
      const auto building_blobs = commonItems::blobList(input_stream).getBlobs();
      for (const auto& blob: building_blobs)
      {
         auto blob_stream = std::stringstream(blob);
         buildings_.emplace_back(blob_stream);
      }
   });
   parser.registerKeyword("income", [this](const std::string&, std::istream& input_stream) {
      income_ = commonItems::singleDouble(input_stream).getDouble();
   });
   parser.registerKeyword("barter_goods", [this](const std::string&, std::istream& input_stream) {
      barter_goods_ = commonItems::singleDouble(input_stream).getDouble();
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}
