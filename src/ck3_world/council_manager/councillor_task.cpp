#include "councillor_task.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "src/ck3_world/id_pointer_pair.hpp"


ck3::CouncillorTask::CouncillorTask(std::istream& input_stream, long long task_id): task_id_(task_id)
{
   commonItems::parser parser;
   parser.registerKeyword("type", [this](const std::string&, std::istream& input_stream) {
      type_ = commonItems::singleString(input_stream).getString();
   });
   parser.registerKeyword("owner", [this](const std::string&, std::istream& input_stream) {
      holder_ = IdPointerPair<Character>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerKeyword("court_owner", [this](const std::string&, std::istream& input_stream) {
      court_owner_ = IdPointerPair<Character>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

void ck3::CouncillorTask::LinkCharacters(const std::map<long long, std::shared_ptr<Character>>& characters)
{
   if (!holder_.has_value())
   {
      // Frozen or broken task
      return;
   }
   if (characters.contains(holder_->GetID()))
   {
      holder_->SetPointer(characters.at(holder_->GetID()));
   }
   else
   {
      throw std::runtime_error("Councillor task " + std::to_string(task_id_) + " has holder (owner) " +
                               std::to_string(holder_->GetID()) + " who doens't exist in save!");
   }
   if (characters.contains(court_owner_.GetID()))
   {
      court_owner_.SetPointer(characters.at(court_owner_.GetID()));
   }
   else
   {
      throw std::runtime_error("Councillor task " + std::to_string(task_id_) + " has court owner " +
                               std::to_string(court_owner_.GetID()) + " who doens't exist in save!");
   }
}