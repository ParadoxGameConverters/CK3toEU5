#ifndef CK3_WORLD_H
#define CK3_WORLD_H

#include <Date.h>

#include "GameVersion.h"
#include "Parser.h"
#include "characters/characters.hpp"
#include "confederations/confederations.hpp"
#include "cultures/cultures.hpp"
#include "dynasties/dynasties.hpp"
#include "flags/flags.hpp"
#include "geography/county_details.hpp"
#include "geography/province_holdings.hpp"
#include "religions/religions.hpp"
#include "src/configuration/configuration.hpp"
#include "titles/landed_titles.hpp"
#include "titles/titles.hpp"

namespace ck3
{
class CK3World
{
  public:
   explicit CK3World(const configuration::Configuration& configuration,
       const commonItems::ConverterVersion& converter_version);

   [[nodiscard]] const auto& GetCultures() const { return cultures_; }

   [[nodiscard]] const auto& GetConversionDate() const { return end_date_; }
   //[[nodiscard]] const auto& GetMods() const { return mods; }
   [[nodiscard]] const auto& GetTitles() const { return titles_; }
   [[nodiscard]] const auto& GetCharacters() const { return characters_; }
   [[nodiscard]] const auto& GetDynasties() const { return dynasties_; }
   [[nodiscard]] const auto& GetReligions() const { return religions_; }
   [[nodiscard]] const auto& GetConfederations() const { return confederations_; }
   [[nodiscard]] const auto& GetCountyDetails() const { return county_details_; }
   [[nodiscard]] const auto& GetLandedTitles() const { return landed_titles_; }
   [[nodiscard]] const auto& GetMetaTitleName() const { return meta_realm_title_; }
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
   void LoadLandedTitles(const configuration::Configuration& configuration);

   // savegame processing

   date end_date_ = date("1444.11.11");
   date start_date_ = date("1.1.1");

   // meta
   std::optional<std::string> meta_realm_title_;

   GameVersion ck3_version_;
   Flags flags_;
   // Mods mods_;

   // world
   Titles titles_;
   ProvinceHoldings province_holdings_;
   Characters characters_;
   Dynasties dynasties_;
   Religions religions_;
   // CoatsOfArms coats;
   CountyDetails county_details_;
   Cultures cultures_;
   // HouseNameScraper houseNameScraper;
   Confederations confederations_;
   // Relations relations;
   // Opinions opinions;
   // Wars wars;
   // Artifacts artifacts;
   // Armies armies;
   // VassalContracts vassalContracts;
   // mappers::NamedColors namedColors;
   // mappers::TraitScraper traitScraper;
   // mappers::LocalizationMapper localizationMapper;

   LandedTitles landed_titles_;
};
}  // namespace ck3

#endif  // CK3_WORLD_H
