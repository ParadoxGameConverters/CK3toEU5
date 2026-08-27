#include "characters.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "character.hpp"
#include "src/ck3_world/council_manager/councillor_tasks.hpp"
#include "src/ck3_world/cultures/cultures.hpp"
#include "src/ck3_world/dynasties/dynasties.hpp"
#include "src/ck3_world/religions/religions.hpp"
#include "src/ck3_world/titles/title.hpp"
#include "src/ck3_world/titles/titles.hpp"

void ck3::Characters::ParseCharacters(std::istream& input_stream)
{
   commonItems::parser character_parser;
   character_parser.registerRegex(R"(\d+)", [this](const std::string& character_id, std::istream& input_stream) {
      const std::shared_ptr<Character> new_character =
          std::make_shared<Character>(input_stream, std::stoll(character_id));
      if (new_character->IsDead())
      {
         characters_dead_.insert(std::make_pair(new_character->GetID(), new_character));
      }
      else
      {
         characters_alive_.insert(std::make_pair(new_character->GetID(), new_character));
      }
      all_characters_.insert(std::make_pair(new_character->GetID(), new_character));
   });
   character_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   character_parser.parseStream(input_stream);
   character_parser.clearRegisteredKeywords();
}

void ck3::Characters::LinkCharacters()
{
   for (const auto& character: characters_alive_)
   {
      character.second->LinkCharacters(all_characters_);
   }
   Log(LogLevel::Info) << "Characters linked.";
}


void ck3::Characters::LinkCultures(const Cultures& cultures)
{
   const auto& culture_map = cultures.GetCultures();
   for (const auto& character: characters_alive_)
   {
      character.second->LinkCulture(culture_map);
   }
   Log(LogLevel::Info) << "Character cultures linked.";
}

void ck3::Characters::LinkFaiths(const Religions& religions)
{
   const auto& faith_map = religions.GetFaiths();
   for (const auto& character: characters_alive_)
   {
      character.second->LinkFaith(faith_map);
   }
   Log(LogLevel::Info) << "Character faiths linked.";
}

void ck3::Characters::LinkHouses(const Dynasties& houses)
{
   const auto& house_map = houses.GetHouses();
   for (const auto& character: characters_alive_)
   {
      character.second->LinkHouse(house_map);
   }
   Log(LogLevel::Info) << "Character houses linked.";
}

void ck3::Characters::LinkTitles(const Titles& titles, const CouncillorTasks& councillor_tasks)
{
   const auto& title_map = titles.GetTitles();
   // Since titles are locked behind name keys and we'll needs IDs, make a cache.
   std::map<long long, std::shared_ptr<Title>> id_title_map;
   for (const auto& title: title_map)
   {
      id_title_map.insert(std::pair(title.second->GetID(), title.second));
   }

   for (const auto& character: all_characters_)
   {
      character.second->LinkCharacterRealm(id_title_map, councillor_tasks.GetCouncillorTasks());
      character.second->LinkClaims(id_title_map);
   }
   Log(LogLevel::Info) << "Character realms and claims linked.";
}