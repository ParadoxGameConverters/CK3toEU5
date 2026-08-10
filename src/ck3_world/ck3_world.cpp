#include "ck3_world.hpp"
// #include "CommonFunctions.h"
#include "CommonRegexes.h"
#include "Log.h"
// #include "ModLoader/ModFilesystem.h"

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
#include "characters/characters.hpp"
#include "confederations/confederations.hpp"
#include "cultures/cultures.hpp"
#include "dynasties/dynasties.hpp"
#include "external/commonItems/ConverterVersion.h"
#include "flags/flags.hpp"
#include "religions/religions.hpp"
#include "save_melter.hpp"
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

ck3::CK3World::CK3World(const configuration::Configuration& configuration,
    const commonItems::ConverterVersion& converter_version)
{
   Log(LogLevel::Info) << "-> Verifying CK3 save.";
   // TODO: Kmiotek - move this to a seperate class
   SaveMelter melter;
   melter.VerifySave(configuration.GetSaveGamePath());
   SaveData save_game = melter.MeltSave(configuration.GetSaveGamePath(), configuration.GetDebug());
   Log(LogLevel::Progress) << "5 %";

   Log(LogLevel::Info) << "* Parsing Metadata *";
   auto metadata_stream = std::istringstream(save_game.metadata);
   ParseMeta(metadata_stream);
   Log(LogLevel::Progress) << "7 %";

   Log(LogLevel::Info) << "* Parsing Gamestate *";
   auto game_state_stream = std::istringstream(save_game.gamestate);
   ParseGamestate(game_state_stream, converter_version);
   Log(LogLevel::Progress) << "20 %";

   Log(LogLevel::Info) << "* Gamestate Parsing Complete, Weaving Internals *";
   Log(LogLevel::Progress) << "30 %";

   Log(LogLevel::Info) << "*** Good-bye CK3, rest in peace. ***";
   Log(LogLevel::Progress) << "47 %";
}

void ck3::CK3World::ParseGamestate(std::istream& input_stream, const commonItems::ConverterVersion& converter_version)
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
   parser.registerKeyword("landed_titles", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading titles.";
      titles_ = Titles(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << titles_.GetTitles().size() << " titles: " << titles_.GetBaronies().size()
                          << " baronies, " << titles_.GetCounties().size() << " counties, "
                          << titles_.GetDuchies().size() << " duchies, " << titles_.GetKingdoms().size()
                          << " kingdoms, " << titles_.GetEmpires().size() << " empires, "
                          << titles_.GetHegemonies().size() << " hegemonies.";
   });
   // registerKeyword("provinces", [this](const std::string&, std::istream& input_stream) {
   //	Log(LogLevel::Info) << "-> Loading provinces.";
   //	province_holdings_ = ProvinceHoldings(input_stream);
   //	Log(LogLevel::Info) << "<> Loaded " << province_holdings_.getProvinceHoldings().size() << " provinces.";
   // });

   parser.registerKeyword("living", [this](std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading alive characters.";
      characters_.ParseAliveCharacters(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << characters_.GetAliveCharacters().size() << " living characters.";
   });

   parser.registerKeyword("dead_unprunable", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading dead people.";
      characters_.ParseDeadCharacters(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << characters_.GetDeadCharacters().size() << " dead human memories.";
   });
   parser.registerKeyword("dynasties", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading dynasties.";
      dynasties_ = Dynasties(input_stream);
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
      cultures_ = Cultures(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << cultures_.GetCultures().size() << " cultures.";
   });
   parser.registerKeyword("confederation_manager", [this](const std::string&, std::istream& input_stream) {
      Log(LogLevel::Info) << "-> Loading confederations.";
      confederations_ = Confederations(input_stream);
      Log(LogLevel::Info) << "<> Loaded " << confederations_.GetConfederations().size() << " confederations.";
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

void ck3::CK3World::ParseMeta(std::istream& input_stream)
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
