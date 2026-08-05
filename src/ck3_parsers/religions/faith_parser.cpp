#include "faith_parser.hpp"

#include <iostream>
#include <string>

#include "CommonRegexes.h"
#include "ParserHelpers.h"

ck3::FaithParser::FaithParser(std::istream& input_stream, long long faith_id): faith_id_(faith_id)
{
   ParseFaith(input_stream);
}

void ck3::FaithParser::ParseFaith(std::istream& input_stream)
{
   registerKeyword("tag", [this](const std::string&, std::istream& input_stream) {
      tag_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("doctrine", [this](const std::string&, std::istream& input_stream) {
      ParseDoctrine(input_stream);
   });
   registerKeyword("religion", [this](const std::string&, std::istream& input_stream) {
      religion_ = commonItems::singleLlong(input_stream).getLlong();
   });
   registerKeyword("faith_type", [this](const std::string&, std::istream& input_stream) {
      faith_type_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("name", [this](const std::string&, std::istream& input_stream) {
      custom_name_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("adjective", [this](const std::string&, std::istream& input_stream) {
      custom_adjective_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("religious_head", [this](const std::string&, std::istream& input_stream) {
      religious_head_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("desc", [this](const std::string&, std::istream& input_stream) {
      description_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("icon", [this](const std::string&, std::istream& input_stream) {
      icon_path_ = commonItems::singleString(input_stream).getString();
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}

void ck3::FaithParser::ParseDoctrine(std::istream& input_stream)
{
   const std::string doctrine = commonItems::singleString(input_stream).getString();
   if (doctrine.find("tenet") == 0)
   {
      tenets_.insert(doctrine);
   }
   else
   {
      doctrines_.insert(doctrine);
   }
   if (doctrine.contains("unreformed"))
   {  // every unreformed faith has a special doctrine differing in name based on region
      is_reformed_ = false;
   }
}