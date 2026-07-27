#ifndef CK3_WORLD_H
#define CK3_WORLD_H
#include "../mappers/LocalizationMapper/LocalizationMapper.h"
#include "../mappers/NamedColors/NamedColors.h"
#include "../mappers/TraitScraper/TraitScraper.h"
#include "Characters/Characters.h"
#include "CoatsOfArms/CoatOfArms.h"
#include "CoatsOfArms/CoatsOfArms.h"
#include "Confederations/Confederations.h"
#include "ConverterVersion.h"
#include "Cultures/Cultures.h"
#include "Dynasties/Dynasties.h"
#include "Dynasties/HouseNameScraper.h"
#include "Dynasties/Houses.h"
#include "Flags/Flags.h"
#include "GameVersion.h"
#include "Geography/CountyDetails.h"
#include "Geography/ProvinceHoldings.h"
#include "ModLoader/ModLoader.h"
#include "Parser.h"
#include "Artifacts/Artifacts.h"
#include "Relations/Opinions.h"
#include "Armies/Armies.h"
#include "Relations/Relations.h"
#include "VassalContracts/VassalContracts.h"
#include "Religions/Faiths.h"
#include "Religions/Religions.h"
#include "Wars/Wars.h"
#include "Titles/LandedTitles.h"
#include "Titles/Titles.h"
#include <Date.h>

#include "src/configuration/configuration.hpp"

namespace CK3
{
class World: commonItems::parser
{
  public:
	explicit World(const configuration::Configuration& theConfiguration, const commonItems::ConverterVersion& converterVersion);

	[[nodiscard]] const auto& getConversionDate() const { return endDate; }
	[[nodiscard]] const auto& getIndeps() const { return independentTitles; }
	[[nodiscard]] const auto& getMods() const { return mods; }
	[[nodiscard]] const auto& getTitles() const { return titles; }
	[[nodiscard]] const auto& getCharacters() const { return characters; }
	[[nodiscard]] const auto& getDynasties() const { return dynasties; }
	[[nodiscard]] const auto& getHouses() const { return houses; }
	[[nodiscard]] const auto& getFaiths() const { return faiths; }
	[[nodiscard]] const auto& getReligions() const { return religions; }
	[[nodiscard]] const auto& getCultures() const { return cultures; }
	[[nodiscard]] const auto& getConfederations() const { return confederations; }
	[[nodiscard]] const auto& getCountyDetails() const { return countyDetails; }
	[[nodiscard]] const auto& getLandedTitles() const { return landedTitles; }
	[[nodiscard]] auto doesIslamExist() const { return islamExists; }
	[[nodiscard]] const auto& getPlayerTitle() const { return playerTitle; }
	[[nodiscard]] const auto& getMetaTitleName() const { return metaTitleName; }
	[[nodiscard]] const auto& getMetaCoA() const { return metaCoA; }
	[[nodiscard]] const auto& getLocalizationMapper() const { return localizationMapper; }
	[[nodiscard]] const auto& getAlliancePairs() const { return relations.getAlliancePairs(); }
	[[nodiscard]] const auto& getRivalPairs() const { return opinions.getRivalPairs(); }
	[[nodiscard]] const auto& getWars() const { return wars.getWars(); }
	[[nodiscard]] const auto& getArtifacts() const { return artifacts.getArtifacts(); }
	[[nodiscard]] const auto& getMenAtArms() const { return armies.getMenAtArms(); }
	[[nodiscard]] const auto& getVassalContracts() const { return vassalContracts; }

  private:
	void registerKeys(const configuration::Configuration& theConfiguration, const commonItems::ConverterVersion& converterVersion);

	parser metaParser;

	// savegame processing
	void verifySave(const std::filesystem::path& saveGamePath) const;
	void processSave(const std::filesystem::path& saveGamePath, bool debug);

	// pre-parsing prep
	void primeLaFabricaDeColor(const configuration::Configuration& theConfiguration);
	void loadLandedTitles(const configuration::Configuration& theConfiguration);
	void loadCharacterTraits(const configuration::Configuration& theConfiguration);
	void loadHouseNames(const configuration::Configuration& theConfiguration);

	// postparsing weave
	void crosslinkDatabases();

	// CK3World processing
	void checkForIslam();
	void filterIndependentTitles();
	void gatherCourtierNames();
	void congregateDFCounties();
	void congregateDJCounties();
	void filterLandlessTitles();
	void locatePlayerTitle();

	date endDate = date("1444.11.11");
	date startDate = date("1.1.1");
	std::optional<long long> playerID;
	std::optional<std::string> playerTitle;
	std::optional<std::string> metaTitleName;
	std::shared_ptr<CoatOfArms> metaCoA; // the player realm's arms exactly as CK3 displays them
	GameVersion CK3Version;
	Titles titles;
	ProvinceHoldings provinceHoldings;
	Characters characters;
	Dynasties dynasties;
	Houses houses;
	Religions religions;
	Faiths faiths;
	CoatsOfArms coats;
	LandedTitles landedTitles;
	Mods mods;
	Flags flags;
	CountyDetails countyDetails;
	Cultures cultures;
	HouseNameScraper houseNameScraper;
	Confederations confederations;
	Relations relations;
	Opinions opinions;
	Wars wars;
	Artifacts artifacts;
	Armies armies;
	VassalContracts vassalContracts;
	mappers::NamedColors namedColors;
	mappers::TraitScraper traitScraper;
	mappers::LocalizationMapper localizationMapper;

	std::map<std::string, std::shared_ptr<Title>> independentTitles;

	bool islamExists = false;

	struct saveData
	{
		std::string gamestate;
		std::string metadata; // we use this to set up mods before main processing.
		bool parsedMeta = false;
	};
	saveData saveGame;
};
} // namespace CK3

#endif // CK3_WORLD_H
