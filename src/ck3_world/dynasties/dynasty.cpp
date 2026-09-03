#include "dynasty.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include "CommonRegexes.h"
#include "ParserHelpers.h"
#include "src/ck3_world/characters/character.hpp"
#include "src/ck3_world/id_pointer_pair.hpp"

ck3::Dynasty::Dynasty(std::istream& input_stream, long long savegame_dynasty_id):
    savegame_dynasty_id_(savegame_dynasty_id)
{
   ParseDynasty(input_stream);
}

void ck3::Dynasty::ParseDynasty(std::istream& input_stream)
{
   registerKeyword("key", [this](const std::string&, std::istream& input_stream) {
      dynasty_id_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("dynasty_head", [this](const std::string&, std::istream& input_stream) {
      dynasty_head_ = IdPointerPair<Character>(commonItems::singleLlong(input_stream).getLlong());
   });
   registerKeyword("good_for_realm_name", [this](const std::string&, std::istream& input_stream) {
      appropriate_realm_name_ = commonItems::singleString(input_stream).getString() == "yes";
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}

void ck3::Dynasty::LinkCharacters(const std::map<long long, std::shared_ptr<Character>>& characters_map)
{
   if (dynasty_head_.has_value())
   {
      if (characters_map.contains(dynasty_head_->GetID()))
      {
         dynasty_head_->SetPointer(characters_map.at(dynasty_head_->GetID()));
      }
      else
      {
         throw std::runtime_error("Dynasty " + std::to_string(savegame_dynasty_id_) + " has dynasty head " +
                                  std::to_string(dynasty_head_->GetID()) + " which has no definition!");
      }
   }
}
