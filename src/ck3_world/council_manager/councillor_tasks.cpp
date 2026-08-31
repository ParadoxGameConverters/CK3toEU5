#include "councillor_tasks.hpp"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "councillor_task.hpp"
#include "src/ck3_world/characters/characters.hpp"

ck3::CouncillorTasks::CouncillorTasks(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("active", [this](const std::string&, std::istream& input_stream) {
      ParseActive(input_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}


void ck3::CouncillorTasks::ParseActive(std::istream& input_stream)
{
   commonItems::parser tasks_parser;
   tasks_parser.registerRegex(R"(\d+)", [this](const std::string& task_id, std::istream& input_stream) {
      const auto task_blob_as_string = commonItems::stringOfItem(input_stream).getString();
      if (!task_blob_as_string.contains("{"))  // task=none
      {
         return;
      }
      auto task_stream = std::stringstream(task_blob_as_string);
      const auto new_task = std::make_shared<CouncillorTask>(task_stream, std::stoll(task_id));
      tasks_.insert(std::make_pair(new_task->GetID(), new_task));
   });
   tasks_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   tasks_parser.parseStream(input_stream);
   tasks_parser.clearRegisteredKeywords();
}

void ck3::CouncillorTasks::LinkCharacters(const Characters& characters)
{
   const auto& character_map = characters.GetAllCharacters();
   for (const auto& councillor_task: tasks_)
   {
      councillor_task.second->LinkCharacters(character_map);
   }
   Log(LogLevel::Debug) << "Councillor tasks linked.";
}