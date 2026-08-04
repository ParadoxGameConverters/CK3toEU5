#include "flags.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "flag.hpp"

// This file loads data flags ("flag_re_restored_antioch"), not graphical flags.
// Since game uses this type of syntax:
//		type=flag
//		flag="flag_formed_kingdom_of_aragon"
// we'll be loading only those flags that have type = flag set. Unsure if any other flag type exists.
// Flags in "data" section of save are ignored - those appear to be gameplay-session related and not really global.
//
// Flags are further complicated by being bundled in data groups. As of yet we do not know if
// name="unavailable_unique_decisions" { flag="flag_formed_kingdom_of_aragon" }
// means aragon is already formed, thus decision is unavailable (giving proper semantics to the flag we scrape) or
// aragon forming decision is simply unavailable, in which case the flag means the opposite of what we assume it to be.

ck3::Flags::Flags(std::istream& input_stream)
{
   ParseVariables(input_stream);
}

void ck3::Flags::ParseVariables(std::istream& input_stream)
{
   registerKeyword("list", [this](const std::string&, std::istream& input_stream) {
      for (const auto& blob: commonItems::blobList(input_stream).getBlobs())
      {
         auto blob_stream = std::stringstream(blob);
         ParseDecisionFlag(blob_stream);
      }
   });
   registerKeyword("data", [this](const std::string&, std::istream& input_stream) {
      const commonItems::parser blob_parser;
      for (const auto& blob: commonItems::blobList(input_stream).getBlobs())
      {
         auto blob_stream = std::stringstream(blob);
         const Flag new_flag(blob_stream);
         flags_.insert(std::make_pair(new_flag.GetName(), new_flag));
      }
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parseStream(input_stream);
   clearRegisteredKeywords();
}

void ck3::Flags::ParseDecisionFlag(std::istream& input_stream)
{
   commonItems::parser blob_parser;
   bool writing_to_unavailable_decisions = false;
   blob_parser.registerKeyword("name",
       [&writing_to_unavailable_decisions](const std::string&, std::istream& input_stream) {
          const std::string name = commonItems::singleString(input_stream).getString();
          if (name == "unavailable_unique_decisions")
          {
             writing_to_unavailable_decisions = true;
          }
       });
   blob_parser.registerKeyword("item",
       [this, &writing_to_unavailable_decisions](const std::string&, std::istream& input_stream) {
          if (writing_to_unavailable_decisions)
          {
             commonItems::parser decision_flag_parser;
             decision_flag_parser.registerKeyword("flag", [this](const std::string&, std::istream& input_stream) {
                unavailable_unique_decisions_.insert(commonItems::singleString(input_stream).getString());
             });
             decision_flag_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
             decision_flag_parser.parseStream(input_stream);
             decision_flag_parser.clearRegisteredKeywords();
          }
       });
   blob_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   blob_parser.parseStream(input_stream);
   blob_parser.clearRegisteredKeywords();
}
