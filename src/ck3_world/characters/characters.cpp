#include "characters.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "character.hpp"
#include "src/ck3_world/cultures/cultures.hpp"
#include "src/ck3_world/id_pointer_pair.hpp"
#include "src/ck3_world/religions/religions.hpp"

void ck3::Characters::ParseAliveCharacters(std::istream& input_stream)
{
   commonItems::parser character_parser;
   character_parser.registerRegex(R"(\d+)", [this](const std::string& character_id, std::istream& input_stream) {
      const std::shared_ptr<Character> new_character =
          std::make_shared<Character>(input_stream, std::stoll(character_id));
      characters_alive_.insert(std::make_pair(new_character->GetID(), new_character));
      all_characters_.insert(std::make_pair(new_character->GetID(), new_character));
   });
   character_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   character_parser.parseStream(input_stream);
   character_parser.clearRegisteredKeywords();
}

void ck3::Characters::ParseDeadCharacters(std::istream& input_stream)
{
   commonItems::parser character_parser;
   character_parser.registerRegex(R"(\d+)", [this](const std::string& character_id, std::istream& input_stream) {
      const std::shared_ptr<Character> new_character =
          std::make_shared<Character>(input_stream, std::stoll(character_id));
      characters_dead_.insert(std::make_pair(new_character->GetID(), new_character));
      all_characters_.insert(std::make_pair(new_character->GetID(), new_character));
   });
   character_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   character_parser.parseStream(input_stream);
   character_parser.clearRegisteredKeywords();
}

void ck3::Characters::LinkCharacters()
{
   for (auto character_iterator = characters_alive_.begin(); character_iterator != characters_alive_.end();
       ++character_iterator)
   {
      try
      {
         auto spouse = (*character_iterator).second->GetSpouse();
         if (spouse.has_value())
         {
            if (all_characters_.contains(spouse->GetID()))
            {
               spouse->SetPointer(all_characters_.at(spouse->GetID()));
            }
            else
            {
               (*character_iterator).second->RemoveSpouse();  // dead and pruned spouse
            }
         }
         auto employer = (*character_iterator).second->GetEmployer();
         if (employer.has_value() && all_characters_.contains(employer->GetID()))
         {
            if (all_characters_.contains(employer->GetID()))
            {
               employer->SetPointer(all_characters_.at(employer->GetID()));
            }
            else
            {
               (*character_iterator).second->RemoveEmployer();  // dead and pruned employer ??
            }
         }
         auto suzerain = (*character_iterator).second->GetSuzerain();
         if (suzerain.has_value() && all_characters_.contains(suzerain->GetID()))
         {
            if (all_characters_.contains(suzerain->GetID()))
            {
               suzerain->SetPointer(all_characters_.at(suzerain->GetID()));
            }
            else
            {
               (*character_iterator).second->RemoveSuzerain();  // ... ok ck3
            }
         }
         for (auto& child: (*character_iterator).second->GetChildren())
         {
            if (all_characters_.contains(child.GetID()))  // we do not delete info about dead and pruned children,
                                                          // spouses, concubines and knights
            {
               child.SetPointer(all_characters_.at(child.GetID()));
            }
         }
         for (auto& concubine: (*character_iterator).second->GetConcubines())
         {
            if (all_characters_.contains(concubine.GetID()))
            {
               concubine.SetPointer(all_characters_.at(concubine.GetID()));
            }
         }
         for (auto& knight: (*character_iterator).second->GetKnights())
         {
            if (all_characters_.contains(knight.GetID()))
            {
               knight.SetPointer(all_characters_.at(knight.GetID()));
            }
         }
      }
      catch (const std::exception& e)
      {
         Log(LogLevel::Error) << "Error loading character pointers for character " << (*character_iterator).first
                              << "\nError is:\n"
                              << e.what();
      }
   }
   Log(LogLevel::Info) << "Characters linked.";
}


void ck3::Characters::LinkCultures(const Cultures& cultures)
{
   const auto& culture_map = cultures.GetCultures();
   for (const auto& character: characters_alive_)
   {
      if (!character.second->GetCulture())
      {
         // We'll try determining the culture later
         continue;
      }
      if (culture_map.contains(character.second->GetCulture()->GetID()))
      {
         character.second->GetCulture()->SetPointer(culture_map.at(character.second->GetCulture()->GetID()));
      }
      else
      {
         throw std::runtime_error("Character " + std::to_string(character.first) + " has culture " +
                                  std::to_string(character.second->GetCulture()->GetID()) +
                                  " which has no definition!");
      }
   }
   Log(LogLevel::Info) << "Character cultures linked.";
}

void ck3::Characters::LinkFaiths(const Religions& religions)
{
   const auto& faith_map = religions.GetFaiths();
   for (const auto& character: characters_alive_)
   {
      if (!character.second->GetFaith())
      {
         // We'll try determining the faith later
         continue;
      }
      if (faith_map.contains(character.second->GetFaith()->GetID()))
      {
         character.second->GetFaith()->SetPointer(faith_map.at(character.second->GetFaith()->GetID()));
      }
      else
      {
         throw std::runtime_error("Character " + std::to_string(character.first) + " has faith " +
                                  std::to_string(character.second->GetFaith()->GetID()) + " which has no definition!");
      }
   }
   Log(LogLevel::Info) << "Character faiths linked.";
}