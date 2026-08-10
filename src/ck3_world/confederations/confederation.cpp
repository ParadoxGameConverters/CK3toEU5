#include "confederation.hpp"

#include <iostream>
#include <string>

#include "CommonRegexes.h"
#include "ParserHelpers.h"
#include "src/ck3_world/dynasties/house.hpp"
#include "src/ck3_world/id_pointer_pair.hpp"

ck3::Confederation::Confederation(std::istream& input_stream, long long confederation_id):
    confederation_id_(confederation_id)
{
   ParseConfederation(input_stream);
}

void ck3::Confederation::ParseConfederation(std::istream& input_stream)
{
   registerKeyword("houses", [this](std::istream& input_stream) {
      for (auto house_id: commonItems::llongList(input_stream).getLlongs())
      {
         houses_.emplace_back(house_id);
      }
   });
   registerKeyword("leader", [this](std::istream& input_stream) {
      // Despite the bare key name, the leader is a house, matching the houses list.
      leader_house_ = IdPointerPair<House>(commonItems::getLlong(input_stream));
   });
   registerKeyword("name", [this](std::istream& input_stream) {
      name_ = commonItems::getString(input_stream);
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

   parseStream(input_stream);
   clearRegisteredKeywords();
}
