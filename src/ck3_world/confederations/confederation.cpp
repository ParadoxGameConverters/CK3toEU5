#include "confederation.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
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
   registerKeyword("members", [this](std::istream& input_stream) {
      for (auto member_id: commonItems::llongList(input_stream).getLlongs())
      {
         members_.emplace_back(member_id);
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

void ck3::Confederation::LinkCharacters(const std::map<long long, std::shared_ptr<Character>>& characters_map)
{
   for (auto& member: members_)
   {
      if (characters_map.contains(member.GetID()))
      {
         member.SetPointer(characters_map.at(member.GetID()));
      }
      else
      {
         throw std::runtime_error("Confederation " + std::to_string(confederation_id_) + " has character member " +
                                  std::to_string(member.GetID()) + " which has no definition!");
      }
   }
}

void ck3::Confederation::LinkHouses(const std::map<long long, std::shared_ptr<House>>& houses_map)
{
   if (leader_house_.has_value())
   {
      if (houses_map.contains(leader_house_->GetID()))
      {
         leader_house_->SetPointer(houses_map.at(leader_house_->GetID()));
      }
      else
      {
         throw std::runtime_error("Confederation " + std::to_string(confederation_id_) + " has house leader " +
                                  std::to_string(leader_house_->GetID()) + " which has no definition!");
      }
   }
   for (auto& house: houses_)
   {
      if (houses_map.contains(house.GetID()))
      {
         house.SetPointer(houses_map.at(house.GetID()));
      }
      else
      {
         throw std::runtime_error("Confederation " + std::to_string(confederation_id_) + " has house member " +
                                  std::to_string(house.GetID()) + " which has no definition!");
      }
   }
}