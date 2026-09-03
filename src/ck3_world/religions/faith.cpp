#include "faith.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include "CommonRegexes.h"
#include "ParserHelpers.h"
#include "religion.hpp"
#include "src/ck3_world/characters/character.hpp"
#include "src/ck3_world/id_pointer_pair.hpp"

namespace
{
const long long kNoReligiousHeadId = 4294967295;
}  // namespace

ck3::Faith::Faith(std::istream& input_stream, long long faith_id): faith_id_(faith_id)
{
   ParseFaith(input_stream);
}

void ck3::Faith::ParseFaith(std::istream& input_stream)
{
   registerKeyword("tag", [this](const std::string&, std::istream& input_stream) {
      tag_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("doctrine", [this](const std::string&, std::istream& input_stream) {
      ParseDoctrine(input_stream);
   });
   registerKeyword("religion", [this](const std::string&, std::istream& input_stream) {
      religion_ = IdPointerPair<Religion>(commonItems::singleLlong(input_stream).getLlong());
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
      const long long religious_head_id = commonItems::singleLlong(input_stream).getLlong();
      if (religious_head_id == kNoReligiousHeadId)
      {
         religious_head_ = std::nullopt;
      }
      else
      {
         religious_head_ = IdPointerPair<Character>(religious_head_id);
      }
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

void ck3::Faith::ParseDoctrine(std::istream& input_stream)
{
   const std::string doctrine = commonItems::singleString(input_stream).getString();
   if (doctrine.starts_with("tenet"))
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

void ck3::Faith::LinkReligiousHead(const std::map<long long, std::shared_ptr<Character>>& character_map)
{
   if (religious_head_.has_value())
   {
      if (character_map.contains(religious_head_->GetID()))
      {
         religious_head_->SetPointer(character_map.at(religious_head_->GetID()));
      }
      else
      {
         throw std::runtime_error("Faith " + std::to_string(faith_id_) + " has religious head " +
                                  std::to_string(religious_head_->GetID()) + " that doens't exist in save!");
      }
   }
}

void ck3::Faith::LinkReligion(const std::map<long long, std::shared_ptr<Religion>>& religion_map)
{
   if (religion_map.contains(religion_.GetID()))
   {
      religion_.SetPointer(religion_map.at(religion_.GetID()));
   }
   else
   {
      throw std::runtime_error("Faith " + std::to_string(faith_id_) + " belongs to a religion " +
                               std::to_string(religion_.GetID()) + " that doens't exist in save!");
   }
}