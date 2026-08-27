#include "character_realm.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "src/ck3_world/council_manager/councillor_task.hpp"
#include "src/ck3_world/id_pointer_pair.hpp"


ck3::CharacterRealm::CharacterRealm(std::istream& input_stream)
{
   ParseLandedData(input_stream);
}

void ck3::CharacterRealm::ParseLandedData(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("vassal_power_value", [this](const std::string&, std::istream& input_stream) {
      vassal_power_ = commonItems::singleDouble(input_stream).getDouble();
   });
   parser.registerKeyword("government", [this](const std::string&, std::istream& input_stream) {
      government_type_ = commonItems::singleString(input_stream).getString();
   });
   parser.registerKeyword("laws", [this](const std::string&, std::istream& input_stream) {
      const auto& laws_list = commonItems::stringList(input_stream).getStrings();
      laws_ = std::set(laws_list.begin(), laws_list.end());
   });
   parser.registerKeyword("realm_capital", [this](const std::string&, std::istream& input_stream) {
      realm_capital_ = IdPointerPair<Title>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerKeyword("domain", [this](const std::string&, std::istream& input_stream) {
      for (auto title_id: commonItems::llongList(input_stream).getLlongs())
      {
         domain_.emplace_back(title_id);
      }
   });
   parser.registerKeyword("council", [this](const std::string&, std::istream& input_stream) {
      for (auto councillor_task_id: commonItems::llongList(input_stream).getLlongs())
      {
         council_.emplace_back(councillor_task_id);
      }
   });
   parser.registerKeyword("royal_court", [this](const std::string&, std::istream& input_stream) {
      ParseCourtData(input_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

void ck3::CharacterRealm::ParseCourtData(std::istream& input_stream)
{
   commonItems::parser court_parser;
   court_parser.registerKeyword("language", [this](const std::string&, std::istream& input_stream) {
      court_language_ = commonItems::singleString(input_stream).getString();
   });
   court_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   court_parser.parseStream(input_stream);
   court_parser.clearRegisteredKeywords();
}

void ck3::CharacterRealm::Link(const std::map<long long, std::shared_ptr<Title>>& id_title_map,
    const std::map<long long, std::shared_ptr<CouncillorTask>>& tasks,
    long long character_id)
{
   std::vector<IdPointerPair<CouncillorTask>> replacement_council;
   for (auto& councillor_task: council_)
   {
      if (tasks.contains(councillor_task.GetID()))
      {
         const auto& task = tasks.at(councillor_task.GetID());
         if (task->GetCourtOwner().GetID() != character_id)
         {
            Log(LogLevel::Warning) << "Task " << task->GetID() << " claims different court_owner "
                                   << task->GetCourtOwner().GetID() << " than the councillor's realm owner "
                                   << character_id;
         }
         replacement_council.emplace_back(councillor_task.GetID(), task);
      }
      else
      {
         // paradox interactive...
         Log(LogLevel::Debug) << "Missing councillor task " << councillor_task.GetID() << " when linking realm of "
                              << character_id;
      }
   }
   council_ = replacement_council;
   for (auto& title: domain_)
   {
      if (id_title_map.contains(title.GetID()))
      {
         title.SetPointer(id_title_map.at(title.GetID()));
      }
      else
      {
         throw std::runtime_error("Character " + std::to_string(character_id) + " domain title " +
                                  std::to_string(title.GetID()) + " with no definition!");
      }
   }
   if (realm_capital_.has_value())  // no capital for realms consisting only of noble family and/or ceremonial titles
   {
      if (id_title_map.contains(realm_capital_->GetID()))
      {
         realm_capital_->SetPointer(id_title_map.at(realm_capital_->GetID()));
      }
      else
      {
         throw std::runtime_error("Character " + std::to_string(character_id) + " realm capital title " +
                                  std::to_string(realm_capital_->GetID()) + " with no definition.");
      }
   }
}
