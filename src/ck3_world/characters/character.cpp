#include "character.hpp"

#include <iostream>
#include <sstream>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "character_realm.hpp"
#include "src/ck3_world/id_pointer_pair.hpp"


namespace
{
const int kMinSkillsArraySize = 5;
}  // namespace

ck3::Character::Character(std::istream& input_stream, long long character_id): character_id_(character_id)
{
   ParseCharacter(input_stream);
}

void ck3::Character::ParseCharacter(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("first_name", [this](const std::string&, std::istream& input_stream) {
      name_ = commonItems::singleString(input_stream).getString();
   });
   parser.registerKeyword("birth", [this](const std::string&, std::istream& input_stream) {
      birth_date_ = date(commonItems::singleString(input_stream).getString());
   });
   parser.registerKeyword("dead_data", [this](const std::string&, std::istream& input_stream) {
      ParseDeadData(input_stream);
   });
   parser.registerKeyword("alive_data", [this](const std::string&, std::istream& input_stream) {
      ParseAliveData(input_stream);
   });
   parser.registerKeyword("culture", [this](const std::string&, std::istream& input_stream) {
      culture_ = IdPointerPair<Culture>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerKeyword("faith", [this](const std::string&, std::istream& input_stream) {
      faith_ = IdPointerPair<Faith>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerKeyword("dynasty_house", [this](const std::string&, std::istream& input_stream) {
      house_ = IdPointerPair<House>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerKeyword("skill", [this](const std::string&, std::istream& input_stream) {
      const auto& skills_list = commonItems::intList(input_stream).getInts();
      if (skills_list.size() >= kMinSkillsArraySize)
      {
         skills_.diplomacy = skills_list[0];
         skills_.martial = skills_list[1];
         skills_.stewardship = skills_list[2];
         skills_.intrigue = skills_list[3];
         skills_.learning = skills_list[4];
         if (skills_list.size() > kMinSkillsArraySize)
         {
            skills_.prowess = skills_list[kMinSkillsArraySize];
         }
      }
      else
      {
         Log(LogLevel::Error) << "Character " << character_id_
                              << " has a malformed skills block! Size: " << skills_list.size();
      }
   });
   parser.registerKeyword("traits", [this](const std::string&, std::istream& input_stream) {
      for (const auto trait_id: commonItems::intList(input_stream).getInts())
      {
         traits_.insert(trait_id);
      }
   });
   parser.registerKeyword("obedience_target", [this](const std::string&, std::istream& input_stream) {
      suzerain_ = IdPointerPair<Character>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerKeyword("court_data", [this](const std::string&, std::istream& input_stream) {
      ParseCourtData(input_stream);
   });
   parser.registerKeyword("female", [this](const std::string&, std::istream& input_stream) {
      female_ = commonItems::singleString(input_stream).getString() == "yes";
   });
   parser.registerKeyword("landed_data", [this](const std::string&, std::istream& input_stream) {
      realm_ = CharacterRealm(input_stream);
   });
   parser.registerKeyword("family_data", [this](const std::string&, std::istream& input_stream) {
      ParseFamilyData(input_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

void ck3::Character::ParseDeadData(std::istream& input_stream)
{
   commonItems::parser dead_data_parser;
   dead_data_parser.registerKeyword("date", [this](const std::string&, std::istream& input_stream) {
      death_date_ = date(commonItems::singleString(input_stream).getString());
   });
   dead_data_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   dead_data_parser.parseStream(input_stream);
   dead_data_parser.clearRegisteredKeywords();
}

void ck3::Character::ParseAliveData(std::istream& input_stream)
{
   commonItems::parser alive_data_parser;
   alive_data_parser.registerKeyword("piety", [this](const std::string&, std::istream& input_stream) {
      piety_ = RetrieveAccumulated(input_stream);
   });
   alive_data_parser.registerKeyword("prestige", [this](const std::string&, std::istream& input_stream) {
      prestige_ = RetrieveAccumulated(input_stream);
   });
   alive_data_parser.registerKeyword("influence", [this](const std::string&, std::istream& input_stream) {
      influence_ = RetrieveAccumulated(input_stream);
   });
   alive_data_parser.registerKeyword("merit", [this](const std::string&, std::istream& input_stream) {
      merit_ = RetrieveAccumulated(input_stream);
   });
   alive_data_parser.registerKeyword("gold", [this](const std::string&, std::istream& input_stream) {
      ParseGold(input_stream);
   });
   alive_data_parser.registerKeyword("claim", [this](const std::string&, std::istream& input_stream) {
      const auto blob_list = commonItems::blobList(input_stream).getBlobs();
      for (const auto& blob: blob_list)
      {
         auto blob_stream = std::stringstream(blob);
         ParseClaim(blob_stream);
      }
   });
   alive_data_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   alive_data_parser.parseStream(input_stream);
   alive_data_parser.clearRegisteredKeywords();
}

double ck3::Character::RetrieveAccumulated(std::istream& input_stream)
{
   commonItems::parser accumulated_parser;
   double accumulated = 0;
   accumulated_parser.registerKeyword("accumulated", [&accumulated](const std::string&, std::istream& input_stream) {
      accumulated = commonItems::singleDouble(input_stream).getDouble();
   });
   accumulated_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   accumulated_parser.parseStream(input_stream);
   accumulated_parser.clearRegisteredKeywords();
   return accumulated;
}

void ck3::Character::ParseGold(std::istream& input_stream)
{
   commonItems::parser gold_parser;
   gold_parser.registerKeyword("value", [this](const std::string&, std::istream& input_stream) {
      gold_ = commonItems::singleDouble(input_stream).getDouble();
   });
   gold_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   gold_parser.parseStream(input_stream);
   gold_parser.clearRegisteredKeywords();
}

void ck3::Character::ParseCourtData(std::istream& input_stream)
{
   commonItems::parser court_data_parser;
   court_data_parser.registerKeyword("employer", [this](const std::string&, std::istream& input_stream) {
      employer_ = IdPointerPair<Character>(commonItems::singleLlong(input_stream).getLlong());
   });
   court_data_parser.registerKeyword("knight", [this](const std::string&, std::istream& input_stream) {
      knight_ = commonItems::singleString(input_stream).getString() == "yes";
   });
   court_data_parser.registerRegex("council_task|special_council_tasks",
       [this](const std::string&, std::istream& input_stream) {
          councilor_ = true;
          commonItems::ignoreItem("unused", input_stream);
       });
   court_data_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   court_data_parser.parseStream(input_stream);
   court_data_parser.clearRegisteredKeywords();
}

void ck3::Character::ParsePlayableData(std::istream& input_stream)
{
   commonItems::parser playable_data_parser;
   playable_data_parser.registerKeyword("knights", [this](const std::string&, std::istream& input_stream) {
      for (auto character_id: commonItems::llongList(input_stream).getLlongs())
      {
         knights_.emplace_back(character_id);
      }
   });
   playable_data_parser.registerKeyword("legitimacy", [this](const std::string&, std::istream& input_stream) {
      legitimacy_ = commonItems::singleDouble(input_stream).getDouble();
   });
   playable_data_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   playable_data_parser.parseStream(input_stream);
   playable_data_parser.clearRegisteredKeywords();
}

void ck3::Character::ParseFamilyData(std::istream& input_stream)
{
   commonItems::parser family_data_parser;
   family_data_parser.registerKeyword("primary_spouse", [this](const std::string&, std::istream& input_stream) {
      primary_spouse_ = IdPointerPair<Character>(commonItems::singleLlong(input_stream).getLlong());
   });
   family_data_parser.registerKeyword("spouse", [this](const std::string&, std::istream& input_stream) {
      spouses_.emplace_back(commonItems::singleLlong(input_stream).getLlong());
   });
   family_data_parser.registerKeyword("concubine", [this](const std::string&, std::istream& input_stream) {
      concubines_.emplace_back(commonItems::singleLlong(input_stream).getLlong());
   });
   family_data_parser.registerKeyword("child", [this](const std::string&, std::istream& input_stream) {
      for (const auto child_id: commonItems::llongList(input_stream).getLlongs())
      {
         children_.emplace_back(child_id);
      }
   });
   family_data_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   family_data_parser.parseStream(input_stream);
   family_data_parser.clearRegisteredKeywords();
}

void ck3::Character::ParseClaim(std::istream& input_stream)
{
   commonItems::parser claims_parser;
   claims_parser.registerKeyword("title", [this](const std::string&, std::istream& input_stream) {
      claims_.emplace_back(commonItems::singleLlong(input_stream).getLlong());
   });
   claims_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   claims_parser.parseStream(input_stream);
   claims_parser.clearRegisteredKeywords();
}
