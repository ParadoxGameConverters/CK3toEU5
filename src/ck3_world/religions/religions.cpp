#include "religions.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "faith.hpp"
#include "religion.hpp"

ck3::Religions::Religions(std::istream& input_stream)
{
   ParseReligions(input_stream);
}

void ck3::Religions::ParseReligions(std::istream& input_stream)
{
   registerKeyword("religions", [this](const std::string&, std::istream& input_stream) {
      ParseOnlyReligions(input_stream);
   });
   registerKeyword("faiths", [this](const std::string&, std::istream& input_stream) {
      ParseFaiths(input_stream);
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}

void ck3::Religions::ParseOnlyReligions(std::istream& input_stream)
{
   commonItems::parser religions_parser;
   religions_parser.registerRegex(R"(\d+)", [this](const std::string& religion_id, std::istream& input_stream) {
      const std::shared_ptr<Religion> new_religion = std::make_shared<Religion>(input_stream, std::stoll(religion_id));
      religions_.insert(std::make_pair(new_religion->GetID(), new_religion));
   });
   religions_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   religions_parser.parseStream(input_stream);
   religions_parser.clearRegisteredKeywords();
}

void ck3::Religions::ParseFaiths(std::istream& input_stream)
{
   commonItems::parser faiths_parser;
   faiths_parser.registerRegex(R"(\d+)", [this](const std::string& faith_id, std::istream& input_stream) {
      const std::shared_ptr<Faith> new_faith = std::make_shared<Faith>(input_stream, std::stoll(faith_id));
      faiths_.insert(std::make_pair(new_faith->GetID(), new_faith));
   });
   faiths_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   faiths_parser.parseStream(input_stream);
   faiths_parser.clearRegisteredKeywords();
}