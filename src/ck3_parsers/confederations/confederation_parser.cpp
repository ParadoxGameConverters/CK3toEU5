#include "confederation_parser.hpp"

#include <iostream>

#include "CommonRegexes.h"
#include "ParserHelpers.h"

ck3::ConfederationParser::ConfederationParser(std::istream& input_stream, long long confederation_id):
    confederation_id_(confederation_id)
{
   ParseConfederation(input_stream);
}

void ck3::ConfederationParser::ParseConfederation(std::istream& input_stream)
{
   registerKeyword("houses", [this](std::istream& input_stream) {
      houses_ = commonItems::getLlongs(input_stream);
   });
   registerKeyword("leader", [this](std::istream& input_stream) {
      // Despite the bare key name, the leader is a house, matching the houses list.
      leader_house_ = commonItems::getLlong(input_stream);
   });
   registerKeyword("name", [this](std::istream& input_stream) {
      name_ = commonItems::getString(input_stream);
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

   parseStream(input_stream);
   clearRegisteredKeywords();
}
