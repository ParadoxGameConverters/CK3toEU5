#include "save_parser.hpp"
// #include "CommonFunctions.h"
#include "CommonRegexes.h"
#include "Log.h"
// #include "ModLoader/ModFilesystem.h"
#include <external/rakaly/rakaly.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "OSCompatibilityLayer.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "characters/character_parser.hpp"
#include "confederations/confederation_parser.hpp"
#include "cultures/culture_parser_map.hpp"
#include "dynasties/dynasties_parser.hpp"
#include "external/commonItems/ConverterVersion.h"
#include "flags/flags.hpp"
#include "religions/religions_parser.hpp"
#include "src/configuration/configuration.hpp"

namespace
{
// CK3 keeps its actual game data under <install>/game/. Older layouts (and test fixtures) may point directly at the
// data root.
std::filesystem::path GetCK3GameDirectory(const configuration::Configuration& configuration)
{
   const auto& ck3_path = configuration.GetCK3Directory();
   if (commonItems::DoesFolderExist(ck3_path / "game"))
   {
      return ck3_path / "game";
   }
   return ck3_path;
}
}  // namespace

ck3::SaveParser::SaveParser(const configuration::Configuration& configuration,
    const commonItems::ConverterVersion& converter_version)
{
   Log(LogLevel::Info) << "-> Verifying CK3 save.";
   VerifySave(configuration.GetSaveGamePath());
   ProcessSave(configuration.GetSaveGamePath(), configuration.GetDebug());
   Log(LogLevel::Progress) << "5 %";

   Log(LogLevel::Info) << "* Parsing Metadata *";
   auto metadata_stream = std::istringstream(save_game_.metadata);
   ParseMeta(metadata_stream);
   Log(LogLevel::Progress) << "7 %";

   Log(LogLevel::Info) << "* Parsing Gamestate *";
   auto game_state_stream = std::istringstream(save_game_.gamestate);
   ParseGamestate(game_state_stream, converter_version);
   Log(LogLevel::Progress) << "20 %";

   Log(LogLevel::Info) << "* Gamestate Parsing Complete, Weaving Internals *";
   Log(LogLevel::Progress) << "30 %";

   Log(LogLevel::Info) << "*** Good-bye CK3, rest in peace. ***";
   Log(LogLevel::Progress) << "47 %";
}

void ck3::SaveParser::ParseGamestate(std::istream& input_stream, const commonItems::ConverterVersion& converter_version)
{
   Log(LogLevel::Info) << "*** Hello CK3, Deus Vult! ***";

   commonItems::parser parser;

   parser.registerRegex("SAV.*", [](const std::string&, std::istream&) {
   });
   parser.registerKeyword("date", [this](const std::string&, std::istream& input_stream) {
      const commonItems::singleString date_string(input_stream);
      end_date_ = date(date_string.getString());
   });
   parser.registerKeyword("bookmark_date", [this](const std::string&, std::istream& input_stream) {
      const commonItems::singleString start_date_string(input_stream);
      start_date_ = date(start_date_string.getString());
   });
   parser.registerKeyword("version",
       [this, converter_version](const std::string&,  // NOLINT : issues with parser error handling
           std::istream& input_stream) {
          const commonItems::singleString version_string(input_stream);
          ck3_version_ = GameVersion(version_string.getString());
          Log(LogLevel::Info) << "<> Savegame version: " << version_string.getString();

          if (converter_version.getMinSource() > ck3_version_)
          {
             Log(LogLevel::Error) << "Converter requires a minimum save from v"
                                  << converter_version.getMinSource().toShortString();
             throw std::runtime_error("Savegame vs converter version mismatch!");
          }
          if (!converter_version.getMaxSource().isLargerishThan(ck3_version_))
          {
             Log(LogLevel::Error) << "Converter requires a maximum save from v"
                                  << converter_version.getMaxSource().toShortString();
             throw std::runtime_error("Savegame vs converter version mismatch!");
          }
       });

   parser.registerKeyword("variables", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading variable flags.";
      flags_ = Flags(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << flags_.GetFlags().size() << " variable flags and "
                          << flags_.GetUnavailableDecisionFlags().size() << " unavailable decision flags.";
   });
   // registerKeyword("landed_titles", [this](const std::string&, std::istream& input_stream) {
   //	Log(LogLevel::Info) << "-> Loading titles.";
   //	titles = Titles(input_stream);
   //	const auto& counter = titles.getCounter();
   //	Log(LogLevel::Info) << "<> Loaded " << titles.getTitles().size() << " titles: " << counter[0] << "b " <<
   // counter[1] << "c " << counter[2] << "d "
   //							  << counter[3] << "k " << counter[4] << "e " << counter[5] <<
   //"h" << counter[6] << " dynamics.";
   // });
   // registerKeyword("provinces", [this](const std::string&, std::istream& input_stream) {
   //	Log(LogLevel::Info) << "-> Loading provinces.";
   //	province_holdings_ = ProvinceHoldings(input_stream);
   //	Log(LogLevel::Info) << "<> Loaded " << province_holdings_.getProvinceHoldings().size() << " provinces.";
   // });

   parser.registerKeyword("living", [this](std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading alive characters.";
      commonItems::parser character_parser;
      character_parser.registerRegex(R"(\d+)", [this](const std::string& character_id, std::istream& input_stream) {
         const CharacterParser new_character(input_stream, std::stoll(character_id));
         characters_alive_.insert(std::make_pair(new_character.GetID(), new_character));
      });
      character_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      character_parser.parseStream(input_stream);
      character_parser.clearRegisteredKeywords();
      Log(LogLevel::Info) << "<> Loaded " << characters_alive_.size() << " living characters.";
   });

   parser.registerKeyword("dead_unprunable", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading dead people.";
      commonItems::parser character_parser;
      character_parser.registerRegex(R"(\d+)", [this](const std::string& character_id, std::istream& input_stream) {
         const CharacterParser new_character(input_stream, std::stoll(character_id));
         characters_dead_.insert(std::make_pair(new_character.GetID(), new_character));
      });
      character_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      character_parser.parseStream(input_stream);
      character_parser.clearRegisteredKeywords();
      Log(LogLevel::Info) << "<> Loaded " << characters_dead_.size() << " dead human memories.";
   });
   parser.registerKeyword("dynasties", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading dynasties.";
      dynasties_ = DynastiesMap(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << dynasties_.GetDynasties().size() << " dynasties and "
                          << dynasties_.GetHouses().size() << " houses.";
   });

   parser.registerKeyword("religion", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading religions.";
      religions_ = Religions(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << religions_.GetReligions().size() << " religions and "
                          << religions_.GetFaiths().size() << " faiths.";
   });
   // registerKeyword("county_manager", [this](const std::string&, std::istream& input_stream) {
   //	Log(LogLevel::Info) << "-> Loading county details.";
   //	countyDetails = CountyDetails(input_stream);
   //	Log(LogLevel::Info) << "<> Loaded " << countyDetails.getCountyDetails().size() << " county details.";
   // });
   parser.registerKeyword("culture_manager", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading cultures.";
      cultures_map_ = CultureParserMap(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << cultures_map_.GetCultures().size() << " cultures.";
   });
   parser.registerKeyword("confederation_manager", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading confederations.";
      ParseConfederations(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << confederations_.size() << " confederations.";
   });
   // registerKeyword("relations", [this](const std::string&, std::istream& input_stream) {
   //	Log(LogLevel::Info) << "-> Loading relations.";
   //	relations = Relations(input_stream);
   //	Log(LogLevel::Info) << "<> Loaded " << relations.getAlliancePairs().size() << " alliances.";
   // });
   // registerKeyword("opinions", [this](const std::string&, std::istream& input_stream) {
   //	Log(LogLevel::Info) << "-> Loading opinions.";
   //	opinions = Opinions(input_stream);
   //	Log(LogLevel::Info) << "<> Loaded " << opinions.getRivalPairs().size() << " rivalries.";
   // });
   // registerKeyword("vassal_contracts", [this](const std::string&, std::istream& input_stream) {
   //	Log(LogLevel::Info) << "-> Loading vassal contracts.";
   //	vassalContracts = VassalContracts(input_stream);
   //	Log(LogLevel::Info) << "<> Loaded " << vassalContracts.getContractGroups().size() << " vassal contracts.";
   // });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

void ck3::SaveParser::ParseConfederations(std::istream& input_stream)
{
   commonItems::parser confederation_manager_parser;
   confederation_manager_parser.registerKeyword("database", [this](std::istream& input_stream) {
      commonItems::parser database_parser;
      database_parser.registerRegex(R"(\d+)", [this](const std::string& confederation_id, std::istream& input_stream) {
         const auto confederation_blob_as_string = commonItems::stringOfItem(input_stream).getString();
         if (confederation_blob_as_string == "none")  // disbanded confederation
         {
            return;
         }
         auto confederation_stream = std::stringstream(confederation_blob_as_string);
         const ConfederationParser new_confederation(confederation_stream, std::stoll(confederation_id));
         confederations_.insert(std::make_pair(new_confederation.GetID(), new_confederation));
      });
      database_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      database_parser.parseStream(input_stream);
   });
   confederation_manager_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   confederation_manager_parser.parseStream(input_stream);
   confederation_manager_parser.clearRegisteredKeywords();
}

void ck3::SaveParser::ParseMeta(std::istream& input_stream)
{
   commonItems::parser meta_parser;
   // meta_parser_.registerKeyword("mods", [this, configuration](std::istream& input_stream) {
   //	Log(LogLevel::Info) << "-> Detecting used mods.";
   //	std::set<std::string> seenMods;
   //	for (const auto& path: commonItems::getStrings(input_stream))
   //	{
   //		if (seenMods.contains(path))
   //			continue;
   //		mods_.emplace_back(Mod("", path));
   //		seenMods.emplace(path);
   //	}
   //	Log(LogLevel::Info) << "<> Savegame claims " << mods_.size() << " mods used.";
   //	commonItems::ModLoader modLoader;
   //	modLoader.loadMods(theConfiguration.GetCK3DocDirectory(), mods_);
   //	mods = modLoader.getMods();
   // });
   meta_parser.registerKeyword("meta_title_name", [this](std::istream& input_stream) {
      // The realm name as CK3 displays it (e.g. "the Yamamoto Empire") - dynamic nomad/adventurer
      // titles_ often carry a stale internal name, so this is the better source for the player realm.
      meta_realm_title_ = commonItems::singleString(input_stream).getString();
      Log(LogLevel::Info) << "Meta title name: " << meta_realm_title_.value_or("no meta title");
   });

   // meta_parser_.registerKeyword("meta_coat_of_arms", [this](std::istream& input_stream) {
   //	// The realm arms as CK3 displays them - for dynamic realms these are the house arms, not the title's.
   //	metaCoA = std::make_shared<CoatOfArms>(input_stream, 0);
   // });
   meta_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   meta_parser.parseStream(input_stream);
   meta_parser.clearRegisteredKeywords();
}

void ck3::SaveParser::ProcessSave(const std::filesystem::path& save_game_path, bool debug)
{
   const std::ifstream save_file(save_game_path, std::ios::binary);
   std::stringstream in_stream;
   in_stream << save_file.rdbuf();
   save_game_.gamestate = in_stream.str();

   const auto save = rakaly::parseCk3(save_game_.gamestate);

   if (const auto& melt = save.meltMeta(); melt)
   {
      Log(LogLevel::Info) << "Meta extracted successfully.";
      melt->writeData(save_game_.metadata);
   }
   else
   {
      Log(LogLevel::Warning) << "NO META!";
      if (save.is_binary())
      {
         Log(LogLevel::Error) << "Binary Save and NO META!";
      }
   }

   if (save.is_binary())
   {
      Log(LogLevel::Info) << "Gamestate is binary, melting.";
      const auto& melt = save.melt();
      if (melt.has_unknown_tokens())
      {
         Log(LogLevel::Error) << "Rakaly reports errors while melting ironman save!";
      }

      melt.writeData(save_game_.gamestate);
   }
   else
   {
      Log(LogLevel::Info) << "Gamestate is textual.";
      const auto& melt = save.melt();
      melt.writeData(save_game_.gamestate);
   }

   // if (save.is_binary())
   //{
   //    Log(LogLevel::Info) << "Gamestate is binary, extracting metadata.";
   //    if (const auto& melt = save.meltMeta(); melt)
   //    {
   //       Log(LogLevel::Info) << "Meta extracted successfully.";
   //       melt->writeData(save_game_.metadata);
   //    }
   //    else
   //    {
   //       Log(LogLevel::Error) << "Binary Save and NO META!";
   //    }
   // }
   //  const auto& melt = save.melt();
   //  if (melt.has_unknown_tokens())
   //  {
   //      Log(LogLevel::Error) << "Rakaly reports errors while melting save!";
   //  }
   //  melt.writeData(save_game_.gamestate);

   if (debug)
   {
      Log(LogLevel::Info) << "Debug is active: Dumping metadata and gamestate to txt files.";
      std::ofstream meta_dump("metaDump.txt");
      meta_dump << save_game_.metadata;
      meta_dump.close();

      std::ofstream save_dump("saveDump.txt");
      save_dump << save_game_.gamestate;
      save_dump.close();
   }
}

void ck3::SaveParser::VerifySave(const std::filesystem::path& save_game_path)
{
   std::ifstream save_file(save_game_path, std::ios::binary);
   if (!save_file.is_open())
   {
      throw std::runtime_error("Could not open save! Exiting!");
   }

   char buffer[10];  // NOLINT : multiple rules dislike c arrays, maybe to be fixed in future
   save_file.get(static_cast<char*>(buffer), 4);
   if (buffer[0] != 'S' || buffer[1] != 'A' || buffer[2] != 'V')
   {
      throw std::runtime_error("Savefile of unknown type.");
   }

   save_file.close();
}
