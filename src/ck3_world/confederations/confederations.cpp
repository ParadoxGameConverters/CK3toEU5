#include "confederations.hpp"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "confederation.hpp"


ck3::Confederations::Confederations(std::istream& input_stream)
{
   ParseConfederations(input_stream);
}

void ck3::Confederations::ParseConfederations(std::istream& input_stream)
{
   registerKeyword("database", [this](std::istream& input_stream) {
      commonItems::parser database_parser;
      database_parser.registerRegex(R"(\d+)", [this](const std::string& confederation_id, std::istream& input_stream) {
         const auto confederation_blob_as_string = commonItems::stringOfItem(input_stream).getString();
         if (confederation_blob_as_string == "none")  // disbanded confederation
         {
            return;
         }
         auto confederation_stream = std::stringstream(confederation_blob_as_string);
         const std::shared_ptr<Confederation> new_confederation =
             std::make_shared<Confederation>(confederation_stream, std::stoll(confederation_id));
         confederations_.insert(std::make_pair(new_confederation->GetID(), new_confederation));
      });
      database_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      database_parser.parseStream(input_stream);
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}