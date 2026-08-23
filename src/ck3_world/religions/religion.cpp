#include "religion.hpp"

#include <iostream>
#include <string>

#include "CommonRegexes.h"
#include "ParserHelpers.h"
#include "faith.hpp"

ck3::Religion::Religion(std::istream& input_stream, long long religion_id): religion_id_(religion_id)
{
   ParseReligion(input_stream);
}

void ck3::Religion::ParseReligion(std::istream& input_stream)
{
   registerKeyword("tag", [this](const std::string&, std::istream& input_stream) {
      tag_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("family", [this](const std::string&, std::istream& input_stream) {
      family_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("faiths", [this](const std::string&, std::istream& input_stream) {
      for (auto faith: commonItems::llongList(input_stream).getLlongs())
      {
         faiths_.emplace_back(faith);
      }
   });
   registerKeyword("religion_type", [this](const std::string&, std::istream& input_stream) {
      religion_type_ = commonItems::singleString(input_stream).getString();
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}
