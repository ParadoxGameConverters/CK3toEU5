#include "title.hpp"

#include "../characters/character.hpp"
#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "src/ck3_world/id_pointer_pair.hpp"

ck3::Title::Title(std::istream& input_stream, long long title_id): title_id_(title_id)
{
   ParseTitle(input_stream);
   DetermineLevelAfterParsing();
}

void ck3::Title::ParseTitle(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("title_name_data", [this](const std::string&, std::istream& input_stream) {
      commonItems::parser name_parser;
      name_parser.registerKeyword("name", [this](const std::string&, std::istream& input_stream) {
         name_ = commonItems::singleString(input_stream).getString();
         // if (display_name.find("\x15") != std::string::npos)
         //{
         //    cleanUpDisplayName();
         // }
      });
      name_parser.registerKeyword("adj", [this](const std::string&, std::istream& input_stream) {
         adjective_ = commonItems::singleString(input_stream).getString();
      });
      name_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      name_parser.parseStream(input_stream);
      name_parser.clearRegisteredKeywords();
   });
   parser.registerKeyword("key", [this](const std::string&, std::istream& input_stream) {
      key_ = commonItems::singleString(input_stream).getString();
   });
   parser.registerKeyword("date", [this](const std::string&, std::istream& input_stream) {
      last_holder_change_date_ = date(commonItems::singleString(input_stream).getString());
   });
   parser.registerKeyword("claim", [this](const std::string&, std::istream& input_stream) {
      for (auto claimantID: commonItems::llongList(input_stream).getLlongs())
      {
         claimants_.push_back(IdPointerPair<Character>(claimantID));
      }
   });
   parser.registerKeyword("history_government", [this](const std::string&, std::istream& input_stream) {
      history_government_ = commonItems::singleString(input_stream).getString();
   });
   parser.registerKeyword("theocratic_lease", [this](const std::string&, std::istream& input_stream) {
      theocratic_lease_ = commonItems::singleString(input_stream).getString() == "yes";
   });
   parser.registerKeyword("capital_barony", [this](const std::string&, std::istream& input_stream) {
      county_capital_barony_ = commonItems::singleString(input_stream).getString() == "yes";
   });
   parser.registerKeyword("duchy_capital_barony", [this](const std::string&, std::istream& input_stream) {
      duchy_capital_barony_ = commonItems::singleString(input_stream).getString() == "yes";
   });
   parser.registerKeyword("capital", [this](const std::string&, std::istream& input_stream) {
      capital_county_ = IdPointerPair<Title>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerKeyword("de_facto_liege", [this](const std::string&, std::istream& input_stream) {
      de_facto_liege_ = IdPointerPair<Title>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerKeyword("de_jure_liege", [this](const std::string&, std::istream& input_stream) {
      de_jure_liege_ = IdPointerPair<Title>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerKeyword("de_jure_vassals", [this](const std::string&, std::istream& input_stream) {
      for (auto vassal_id: commonItems::llongList(input_stream).getLlongs())
      {
         de_jure_vassals_.push_back(IdPointerPair<Title>(vassal_id));
      }
   });
   parser.registerKeyword("heir", [this](const std::string&, std::istream& input_stream) {
      for (auto heirID: commonItems::llongList(input_stream).getLlongs())
      {
         heirs_.push_back(IdPointerPair<Character>(heirID));
      }
   });
   parser.registerKeyword("laws", [this](const std::string&, std::istream& input_stream) {
      const auto& laws = commonItems::stringList(input_stream).getStrings();
      laws_ = std::set(laws.begin(), laws.end());
   });
   parser.registerKeyword("holder", [this](const std::string&, std::istream& input_stream) {
      holder_ = IdPointerPair<Character>(commonItems::singleLlong(input_stream).getLlong());
   });
   parser.registerKeyword("succession_election", [this](const std::string&, std::istream& input_stream) {
      commonItems::parser election_parser;
      election_parser.registerKeyword("electors", [this](const std::string&, std::istream& input_stream) {
         for (auto electorID: commonItems::intList(input_stream).getInts())
         {
            electors_.push_back(IdPointerPair<Character>(electorID));
         }
      });
      election_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      election_parser.parseStream(input_stream);
      election_parser.clearRegisteredKeywords();
   });

   parser.registerKeyword("landless", [this](const std::string&, std::istream& input_stream) {
      landless_ = commonItems::singleString(input_stream).getString() == "yes";
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

void ck3::Title::DetermineLevelAfterParsing()
{
   if (key_.starts_with("b_"))
   {
      level_ = Level::kBarony;
      return;
   }
   if (key_.starts_with("c_"))
   {
      level_ = Level::kCounty;
      return;
   }
   if (key_.starts_with("d_"))
   {
      level_ = Level::kDuchy;
      return;
   }
   if (key_.starts_with("k_"))
   {
      level_ = Level::kKingdom;
      return;
   }
   if (key_.starts_with("e_"))
   {
      level_ = Level::kEmpire;
      return;
   }
   if (key_.starts_with("h_"))
   {
      level_ = Level::kHegemony;
      return;
   }
   level_ = Level::kUnknown;
}

bool ck3::Title::DoesTitleExist()
{
   return (holder_.has_value() && last_holder_change_date_ != date(9999, 1, 1));
}