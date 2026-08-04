#ifndef CK3_WORLD_H
#define CK3_WORLD_H

#include <Date.h>

#include "GameVersion.h"
#include "Parser.h"
#include "characters/character_parser.hpp"
#include "cultures/culture_parser_map.hpp"
#include "dynasties/dynasties_parser.hpp"
#include "flags/flags.hpp"
#include "src/configuration/configuration.hpp"

namespace ck3
{
class SaveParser
{
  public:
   explicit SaveParser(const configuration::Configuration& configuration,
       const commonItems::ConverterVersion& converter_version);

   [[nodiscard]] const auto& GetCultures() const { return cultures_map_; }

   [[nodiscard]] const auto& GetConversionDate() const { return end_date_; }
   //[[nodiscard]] const auto& GetIndeps() const { return independentTitles; }
   //[[nodiscard]] const auto& GetMods() const { return mods; }
   //[[nodiscard]] const auto& GetTitles() const { return titles_; }
   [[nodiscard]] const auto& GetCharacters() const { return characters_alive_; }
   [[nodiscard]] const auto& GetDynasties() const { return dynasties_; }
   //[[nodiscard]] const auto& GetFaiths() const { return faiths; }
   //[[nodiscard]] const auto& GetReligions() const { return religions; }
   //
   //[[nodiscard]] const auto& GetConfederations() const { return confederations; }
   //[[nodiscard]] const auto& GetCountyDetails() const { return countyDetails; }
   //[[nodiscard]] const auto& GetLandedTitles() const { return landedTitles; }
   //[[nodiscard]] const auto& GetPlayerTitle() const { return playerTitle; }
   //[[nodiscard]] const auto& GetMetaTitleName() const { return metaTitleName; }
   //[[nodiscard]] const auto& GetMetaCoA() const { return metaCoA; }
   //[[nodiscard]] const auto& GetLocalizationMapper() const { return localizationMapper; }
   //[[nodiscard]] const auto& GetAlliancePairs() const { return relations.getAlliancePairs(); }
   //[[nodiscard]] const auto& GetRivalPairs() const { return opinions.getRivalPairs(); }
   //[[nodiscard]] const auto& GetWars() const { return wars.getWars(); }
   //[[nodiscard]] const auto& GetArtifacts() const { return artifacts.getArtifacts(); }
   //[[nodiscard]] const auto& GetMenAtArms() const { return armies.getMenAtArms(); }
   //[[nodiscard]] const auto& GetVassalContracts() const { return vassalContracts; }

  private:
   void ParseGamestate(std::istream& input_stream, const commonItems::ConverterVersion& converter_version);
   void ParseMeta(std::istream& input_stream);

   // savegame processing
   static void VerifySave(const std::filesystem::path& save_game_path);
   void ProcessSave(const std::filesystem::path& save_game_path, bool debug);

   date end_date_ = date("1444.11.11");
   date start_date_ = date("1.1.1");

   // meta
   std::optional<std::string> meta_realm_title_;

   GameVersion ck3_version_;
   Flags flags_;
   // Mods mods_;

   // world
   // Titles titles_;
   // ProvinceHoldings province_holdings_;
   std::map<long long, CharacterParser> characters_alive_;
   std::map<long long, CharacterParser> characters_dead_;
   DynastiesMap dynasties_;
   // Religions religions;
   // Faiths faiths;
   // CoatsOfArms coats;
   // LandedTitles landedTitles;
   // CountyDetails countyDetails;
   CultureParserMap cultures_map_;
   // HouseNameScraper houseNameScraper;
   // Confederations confederations;
   // Relations relations;
   // Opinions opinions;
   // Wars wars;
   // Artifacts artifacts;
   // Armies armies;
   // VassalContracts vassalContracts;
   // mappers::NamedColors namedColors;
   // mappers::TraitScraper traitScraper;
   // mappers::LocalizationMapper localizationMapper;

   // std::map<std::string, std::shared_ptr<Title>> independentTitles;

   struct SaveData
   {
      std::string gamestate;
      std::string metadata;  // we use this to set up mods before main processing.
      bool parsed_meta = false;
   };
   SaveData save_game_;
};
}  // namespace ck3

#endif  // CK3_WORLD_H
