#include "characters.hpp"

#include "CommonRegexes.h"
#include "ParserHelpers.h"
#include "Parser.h"
#include "character.hpp"

#include <utility>
#include <string>
#include <memory>
#include <iostream>

void ck3::Characters::ParseAliveCharacters(std::istream& input_stream)
{
   commonItems::parser character_parser;
   character_parser.registerRegex(R"(\d+)", [this](const std::string& character_id, std::istream& input_stream) {
      const std::shared_ptr<Character> new_character =
          std::make_shared<Character>(input_stream, std::stoll(character_id));
      characters_alive_.insert(std::make_pair(new_character->GetID(), new_character));
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
   });
   character_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   character_parser.parseStream(input_stream);
   character_parser.clearRegisteredKeywords();
}
