#ifndef EU5_WORLD_H
#define EU5_WORLD_H
#include "Advances.h"
#include "Color.h"
#include "Country.h"
#include "Date.h"
#include "GameDatabase.h"
#include "LocationDefinitions.h"
#include "VanillaPops.h"
#include "VanillaTowns.h"
#include "src/mappers/BuildingMapper/BuildingMapper.h"
#include "src/mappers/CultureMapper/CultureMapper.h"
#include "src/mappers/DevWeightsMapper/DevWeightsMapper.h"
#include "src/mappers/GovernmentMapper/GovernmentMapper.h"
#include "src/mappers/MenAtArmsMapper/MenAtArmsMapper.h"
#include "src/mappers/SubjectTypeMapper/SubjectTypeMapper.h"
#include "src/mappers/LawMapper/LawMapper.h"
#include "src/mappers/ProvinceMapper/ProvinceMapper.h"
#include "src/mappers/ReligionMapper/ReligionMapper.h"
#include "src/mappers/TitleTagMapper/TitleTagMapper.h"
#include "src/mappers/TraitMapper/TraitMapper.h"
#include "src/mappers/UnitMapper/UnitMapper.h"
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace configuration
{
class Configuration;
}

namespace CK3
{
class World;
class Title;
class Character;
class Culture;
class Faith;
} // namespace CK3

namespace EU5
{
// Converted per-location data sourced from the owning CK3 county: pops get recultured to these,
// development feeds the location development bonus, and city holdings grant town/city ranks.
struct LocationDetails
{
	std::string culture;	 // EU5 culture
	std::string religion; // EU5 religion
	int development = 0;	 // CK3 county development
	int buildings = 0;	 // building count in the source CK3 holding
	bool cityHolding = false; // CK3 city holding present, so a candidate for urban rank
	bool town = false;		  // won an urban slot from the quota
	bool city = false;		  // won city rank rather than town
	std::set<std::string> eu5Buildings; // EU5 building types mapped from CK3 holding buildings
};

// A subject relationship between two converted countries.
struct Dependency
{
	std::string overlord;
	std::string subject;
	std::string type; // EU5 subject_type: vassal/tributary/...
};

// An active CK3 war carried into the EU5 war_manager.
struct ConvertedWar
{
	std::string nameKey; // localization key
	std::string rawName; // display name from CK3
	date startDate = date("1337.1.1");
	std::vector<std::string> attackers; // tags, primary first
	std::vector<std::string> defenders;
	// The location the war is fought over. Without a wargoal EU5 can neither score the war nor
	// let anyone peace out of it, and logs errors when it ends.
	std::string goalLocation;
};

// An EU5 religion generated for a custom/reformed CK3 faith without a good vanilla match.
struct GeneratedReligion
{
	std::string name;		// religion key
	std::string rawName; // display name for localization
	std::string group;	// EU5 religion group borrowed from the parent religion
	std::string language;
	std::optional<commonItems::Color> color;
};

// An EU5 culture generated for a dynamic (hybrid/divergent) CK3 culture.
struct GeneratedCulture
{
	std::string name;		// culture key
	std::string rawName; // display name for localization
	std::string language;
	std::vector<std::string> groups;
	std::vector<std::string> gfxTags; // graphical culture tags, borrowed from a vanilla relative
	std::optional<commonItems::Color> color; // the color the culture wore on the CK3 map
};

// Builds the EU5 world (countries with owned locations, rulers, dynasties) from a parsed CK3 world,
// using the configurable mappers and EU5's own game data for validation.
class World
{
  public:
	World(const CK3::World& sourceWorld, const configuration::Configuration& theConfiguration);

	[[nodiscard]] const auto& getCountries() const { return countries; }
	[[nodiscard]] const auto& getDynasties() const { return dynasties; }
	[[nodiscard]] const auto& getLocationDefinitions() const { return locationDefinitions; }
	[[nodiscard]] const auto& getLocationDetails() const { return locationDetails; }
	[[nodiscard]] const auto& getLocationRenames() const { return locationRenames; }
	[[nodiscard]] const auto& getVanillaPops() const { return vanillaPops; }
	[[nodiscard]] const auto& getVanillaTowns() const { return vanillaTowns; }
	[[nodiscard]] const auto& getDependencies() const { return dependencies; }
	[[nodiscard]] const auto& getAlliances() const { return alliances; }
	[[nodiscard]] const auto& getRivalries() const { return rivalries; }
	[[nodiscard]] const auto& getWars() const { return wars; }
	[[nodiscard]] const auto& getGeneratedReligions() const { return generatedReligions; }
	[[nodiscard]] const auto& getGeneratedCultures() const { return generatedCultures; }
	[[nodiscard]] const auto& getHRETag() const { return hreTag; }
	[[nodiscard]] const auto& getPapacyTag() const { return papacyTag; }
	[[nodiscard]] const auto& getConfederations() const { return confederations; }
	[[nodiscard]] const auto& getGameDatabase() const { return gameDatabase; }
	[[nodiscard]] const auto& getUnitMapper() const { return unitMapper; }
	[[nodiscard]] const auto& getDevWeights() const { return devWeights; }
	[[nodiscard]] auto getDroppedLocationCount() const { return droppedLocations; }
	[[nodiscard]] auto getYearOffset() const { return yearOffset; }

  private:
	void loadEU5GameData(const std::filesystem::path& eu5Path);
	void loadMappers();
	void importCountries(const CK3::World& sourceWorld, const configuration::Configuration& theConfiguration);
	std::optional<std::string> importCountry(const std::string& ck3TitleName, const std::shared_ptr<CK3::Title>& title, const CK3::World& sourceWorld);
	// The stages importCountry runs a realm through, in call order.
	void gatherRealmLand(const CK3::Title& title, const std::shared_ptr<CK3::Character>& holder, Country& country);
	void determineCapital(const std::shared_ptr<CK3::Character>& holder, Country& country);
	void resolveCountryIdentity(const CK3::World& sourceWorld, const CK3::Title& title, const std::shared_ptr<CK3::Character>& holder, Country& country) const;
	void setupGovernment(const std::shared_ptr<CK3::Character>& holder, Country& country) const;
	void applySocietalValuesAndTech(const CK3::Title& title, const std::shared_ptr<CK3::Character>& holder, Country& country);
	void rankMinorityCultures(Country& country) const;
	void assignLanguages(Country& country) const;
	void setupRuler(const CK3::Title& title, const std::shared_ptr<CK3::Character>& holder, Country& country, const CK3::World& sourceWorld);
	void recordPastReigns(const CK3::Title& title, const std::shared_ptr<CK3::Character>& holder, Country& country) const;
	void assignConsortAndHeir(const CK3::Title& title, const std::shared_ptr<CK3::Character>& holder, Country& country, const CK3::World& sourceWorld);
	void importClaims(const CK3::World& sourceWorld);
	void importTributaries(const CK3::World& sourceWorld);
	void importAlliances(const CK3::World& sourceWorld);
	void importRivalries(const CK3::World& sourceWorld);
	void importWars(const CK3::World& sourceWorld);
	void importArtifacts(const CK3::World& sourceWorld);
	void importArmies(const CK3::World& sourceWorld);
	[[nodiscard]] std::vector<std::string> gatherWarSide(const std::vector<long long>& participants, const std::string& primaryTag) const;
	[[nodiscard]] std::string findWarGoal(const std::vector<long long>& targetedTitles,
		 const std::vector<std::string>& defenders,
		 const std::map<long long, std::shared_ptr<CK3::Title>>& titlesByID) const;
	void importConfederations(const CK3::World& sourceWorld);
	void classifyLandControl();
	void applyUrbanQuota();
	[[nodiscard]] static std::optional<long long> findDeJureClaimant(const CK3::Title& county);

	[[nodiscard]] std::vector<std::string> getLocationsForCounty(const CK3::Title& county);
	[[nodiscard]] std::string convertCulture(const std::shared_ptr<CK3::Culture>& culture, const std::string& location);
	[[nodiscard]] std::optional<std::string> generateDynamicCulture(const CK3::Culture& culture);
	[[nodiscard]] std::string pickCultureFromGroups(const std::vector<std::string>& groups, const std::string& ck3Language) const;
	[[nodiscard]] std::string convertFaith(const std::shared_ptr<CK3::Faith>& faith, const std::string& location);
	[[nodiscard]] std::string resolveDisplayName(const CK3::World& sourceWorld, const CK3::Title& title) const;
	[[nodiscard]] std::string resolveAdjective(const CK3::World& sourceWorld, const CK3::Title& title, const std::string& displayName) const;
	[[nodiscard]] std::string determineReligion(const std::shared_ptr<CK3::Character>& holder, const std::string& capital);
	[[nodiscard]] std::string determineCulture(const std::shared_ptr<CK3::Character>& holder, const std::string& capital);
	[[nodiscard]] static std::string determineGovernmentCategory(const std::shared_ptr<CK3::Character>& holder);
	[[nodiscard]] static std::string determineAIPersonality(const std::shared_ptr<CK3::Character>& holder, const Country& country);
	void buildCourt(const std::shared_ptr<CK3::Character>& holder, Country& country, const CK3::World& sourceWorld);
	[[nodiscard]] bool isValidOrGeneratedReligion(const std::string& religion) const;
	[[nodiscard]] bool isValidOrGeneratedCulture(const std::string& culture) const;
	[[nodiscard]] ConvertedCharacter convertCharacter(const std::shared_ptr<CK3::Character>& character,
		 const Country& country,
		 const CK3::World& sourceWorld);
	void buildFamily(const std::shared_ptr<CK3::Character>& holder, Country& country, const CK3::World& sourceWorld);
	std::string registerDynasty(const std::shared_ptr<CK3::Character>& character, const Country& country, const CK3::World& sourceWorld);

	mappers::ProvinceMapper provinceMapper;
	mappers::TitleTagMapper tagMapper;
	mappers::ReligionMapper religionMapper;
	mappers::CultureMapper cultureMapper;
	mappers::GovernmentMapper governmentMapper;
	mappers::TraitMapper traitMapper;
	mappers::LawMapper lawMapper;
	mappers::UnitMapper unitMapper;
	mappers::MenAtArmsMapper menAtArmsMapper;
	mappers::SubjectTypeMapper subjectTypeMapper;
	mappers::BuildingMapper buildingMapper;
	mappers::DevWeightsMapper devWeights;

	LocationDefinitions locationDefinitions;
	GameDatabase gameDatabase;
	VanillaPops vanillaPops;
	VanillaTowns vanillaTowns;
	Advances advances;

	std::map<std::string, Country> countries;					// tag -> country
	std::map<std::string, ConvertedDynasty> dynasties;		// dynasty key -> dynasty, shared across countries
	std::map<std::string, LocationDetails> locationDetails; // owned EU5 location -> converted county data
	std::map<std::string, std::string> locationRenames;	 // EU5 location -> player-given CK3 custom name ("Malik City")
	std::map<long long, std::string> rulerTags;				 // ck3 holder character ID -> tag, for diplomacy matching and PU merging
	std::vector<Dependency> dependencies;						 // subject relationships between converted countries
	std::vector<std::pair<std::string, std::string>> alliances;	// allied tag pairs
	std::vector<std::pair<std::string, std::string>> rivalries;	// rival tag pairs
	std::vector<ConvertedWar> wars;
	std::map<std::string, GeneratedReligion> generatedReligions; // religion key -> generated definition
	std::map<std::string, GeneratedCulture> generatedCultures;	// culture key -> generated definition
	std::map<long long, std::string> generatedFaithNames;			// ck3 faith ID -> generated religion key
	std::map<long long, std::string> generatedCultureNames;		// ck3 culture ID -> generated culture key
	std::optional<std::string> hreTag;	  // the converted Holy Roman Empire, if the save has one
	std::optional<std::string> papacyTag; // the converted Papacy, which leads the catholic_church
	std::map<long long, std::string> houseTags; // ck3 house ID -> the tag its head rules, for confederations
	std::vector<std::pair<std::string, std::vector<std::string>>> confederations; // name -> member tags
	std::set<std::string> takenLocations;	  // first-come-first-served location ownership
	int droppedLocations = 0;					  // locations skipped because another country already took them
	int capitalFallbacks = 0;					  // countries whose CK3 realm capital could not be honored
	int techFloorRaises = 0;					  // countries whose CK3-era technology was too low for their setup template's laws
	int yearOffset = 0;							  // shift applied to character dates so the save date aligns with EU5's 1337 start

	// Conversion options, resolved from the configuration once so the deep conversion code
	// does not have to carry it around.
	bool dynamicCultures = true;	// hybrid/divergent CK3 cultures become new EU5 cultures
	bool dynamicReligions = true; // reformed/custom CK3 faiths become new EU5 religions
	bool ck3TechLevels = true;		// technology from the ruling culture's CK3 era, not the setup template
	bool importTreasury = true;	// countries start with their CK3 ruler's personal gold
	bool importActiveWars = true; // active CK3 wars continue in EU5; off = everyone starts at peace
};
} // namespace EU5

#endif // EU5_WORLD_H
