#include "World.h"
#include "Log.h"
#include "src/ck3_world/Characters/Character.h"
#include "src/ck3_world/Characters/CharacterDomain.h"
#include "src/ck3_world/CoatsOfArms/CoatOfArms.h"
#include "src/ck3_world/Confederations/Confederation.h"
#include "src/ck3_world/Cultures/Culture.h"
#include "src/ck3_world/Dynasties/Dynasty.h"
#include "src/ck3_world/Dynasties/House.h"
#include "src/ck3_world/Geography/CountyDetail.h"
#include "src/ck3_world/Geography/ProvinceHolding.h"
#include "src/ck3_world/Religions/Faith.h"
#include "src/ck3_world/Titles/LandedTitles.h"
#include "src/ck3_world/Titles/Title.h"
#include "src/ck3_world/World.h"
#include "src/configuration/configuration.hpp"
#include <algorithm>
#include <cctype>
#include <functional>
#include <ranges>

namespace
{
std::string sanitizeKey(const std::string& rawName)
{
	std::string key;
	for (const auto character: rawName)
	{
		if (std::isalnum(static_cast<unsigned char>(character)))
			key += static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
		else
			key += '_';
	}
	return key;
}

int scaleSkill(int ck3Skill)
{
	return std::clamp(ck3Skill * 4, 5, 95);
}

// Some saved names are localization references ("$dynn_Nakamikado$") or contain them. Resolve
// each through the CK3 localization; when the key is unknown, its readable tail ("Nakamikado")
// still beats shipping the reference.
std::string resolveNestedName(std::string name, const mappers::LocalizationMapper& localizationMapper)
{
	for (auto rounds = 0; rounds < 8 && name.find('$') != std::string::npos; ++rounds)
	{
		const auto first = name.find('$');
		const auto second = name.find('$', first + 1);
		if (second == std::string::npos)
			break;
		auto key = name.substr(first + 1, second - first - 1);
		if (const auto pipe = key.find('|'); pipe != std::string::npos)
			key = key.substr(0, pipe); // formatting instructions we can't honor anyway
		std::string replacement;
		if (const auto& block = localizationMapper.getLocBlockForKey(key); block && !block->english.empty())
			replacement = block->english;
		else
		{
			replacement = key;
			for (const std::string prefix: {"dynnp_", "dynn_", "nick_", "cn_"})
				if (replacement.starts_with(prefix))
				{
					replacement = replacement.substr(prefix.size());
					break;
				}
			std::ranges::replace(replacement, '_', ' ');
		}
		if (replacement.find('$') != std::string::npos)
			std::erase(replacement, '$'); // a self-referential loc would loop forever
		name = name.substr(0, first) + replacement + name.substr(second + 1);
	}
	return name;
}

// CK3 artifact visual types -> EU5 work-of-art types. CK3 qualifies many of its visuals
// ("indian_book", "artifact_scroll"), so match on the substance rather than the whole string.
std::string artTypeForVisual(const std::string& visualType)
{
	const auto looksLike = [&visualType](const std::string& fragment) {
		return visualType.find(fragment) != std::string::npos;
	};
	if (looksLike("scroll"))
		return "poem";
	if (looksLike("book") || looksLike("manuscript") || looksLike("codex"))
		return "scripture";
	if (looksLike("statue") || looksLike("figurine") || looksLike("sculpture") || looksLike("bust"))
		return "statue";
	if (looksLike("painting") || looksLike("tapestry") || looksLike("banner") || looksLike("portrait"))
		return "painting";
	if (looksLike("icon"))
		return "icon";
	return "regalia"; // crowns, thrones, weapons, armor, jewelry...
}

// Nearly every CK3 artifact carries flavour text of its own, which is what the EU5 tooltip should
// show. The few that don't get a line assembled from what the save does record about them.
std::string describeArtifact(const CK3::Artifact& artifact)
{
	auto object = artifact.visualType;
	if (object.starts_with("artifact_"))
		object = object.substr(9);
	std::ranges::replace(object, '_', ' ');
	if (object.empty())
		object = "treasure";

	std::string rarity = "A"; // "common" needs no adjective; the object itself is the point
	if (artifact.rarity == "masterwork")
		rarity = "A masterwork";
	else if (artifact.rarity == "famed")
		rarity = "A famed";
	else if (artifact.rarity == "illustrious")
		rarity = "An illustrious";
	else if (object.front() == 'a' || object.front() == 'e' || object.front() == 'i' || object.front() == 'o' || object.front() == 'u')
		rarity = "An";

	if (artifact.creationDate)
		return rarity + " " + object + ", in the court's keeping since " + std::to_string(artifact.creationDate->getYear()) + ".";
	return rarity + " " + object + " out of the court's own collection.";
}

// EU5 composes displayed country names from the adjective plus a rank word ("$ADJ$ Empire",
// "$ADJ$ Horde"), so an adjective that already carries a rank word doubles up in game
// ("Yamamoto Empire Empire"). Strip any trailing or leading rank word.
std::string stripRankWords(const std::string& name)
{
	static const std::vector<std::string> rankWords = {"Empire",
		 "Kingdom",
		 "Horde",
		 "Khaganate",
		 "Khanate",
		 "Caliphate",
		 "Sultanate",
		 "Tsardom",
		 "Shogunate",
		 "Hegemony",
		 "Confederation",
		 "Dynasty"};
	for (const auto& word: rankWords)
		if (name.starts_with(word + " of ") && name.size() > word.size() + 4)
			return name.substr(word.size() + 4);
	for (const auto& word: rankWords)
		if (name.ends_with(" " + word) && name.size() > word.size() + 1)
			return name.substr(0, name.size() - word.size() - 1);
	return name;
}

// Whether two characters belong to the same dynasty - a cadet branch of one family rather than
// two unrelated houses.
bool shareDynasty(const std::shared_ptr<CK3::Character>& first, const std::shared_ptr<CK3::Character>& second)
{
	if (!first || !second || !first->getHouse().second || !second->getHouse().second)
		return false;
	const auto& firstDynasty = first->getHouse().second->getDynasty();
	const auto& secondDynasty = second->getHouse().second->getDynasty();
	return firstDynasty.second && secondDynasty.second && firstDynasty.first == secondDynasty.first;
}

// The kingdom-tier vassals of an imperial title that can convert as countries of their own, each
// already holding its whole subtree's counties, largest first so the biggest realm gets first pick
// of land two of them both claim.
//
// Crowns the emperor wears himself are not candidates. CK3 files a secondary title under its own
// de jure liege, so such a kingdom looks like a vassal here; splitting it off would import it first
// and register the emperor against it, and the empire would then merge into that kingdom as a
// personal union - surrendering its name, rank, capital and arms to a secondary title.
std::vector<std::shared_ptr<CK3::Title>> splitoffCandidates(const CK3::Title& imperialTitle)
{
	const auto emperorID = imperialTitle.isHolderLinked() ? imperialTitle.getHolder()->first : 0;
	std::vector<std::shared_ptr<CK3::Title>> candidates;
	for (const auto& vassal: imperialTitle.getDFVassals() | std::views::values)
	{
		if (!vassal || vassal->getLevel() != CK3::LEVEL::KINGDOM || !vassal->isHolderLinked())
			continue;
		if (vassal->getHolder()->first == emperorID)
			continue;
		vassal->congregateDFCounties();
		candidates.push_back(vassal);
	}
	std::ranges::sort(candidates, [](const auto& first, const auto& second) {
		return first->getOwnedDFCounties().size() > second->getOwnedDFCounties().size();
	});
	return candidates;
}
} // namespace

EU5::World::World(const CK3::World& sourceWorld, const configuration::Configuration& theConfiguration)
{
	Log(LogLevel::Info) << "*** Hello EU5, dreaming of Byzantium. ***";
	// EU5 campaigns always start on 1337.4.1. Shift all character dates so ages at campaign start match the save.
	yearOffset = 1337 - sourceWorld.getConversionDate().getYear();
	if (yearOffset != 0)
		Log(LogLevel::Info) << "-> Conversion date is " << sourceWorld.getConversionDate() << "; shifting character dates by " << yearOffset << " years.";
	dynamicCultures = theConfiguration.GetDynamicCultures();
	dynamicReligions = theConfiguration.GetDynamicReligions();
	ck3TechLevels = theConfiguration.GetTechSource() != "vanilla";
	importTreasury = theConfiguration.GetTreasuryImport();
	importActiveWars = theConfiguration.GetWarImport();
	loadEU5GameData(theConfiguration.GetEU5Directory());
	loadMappers();
	importCountries(sourceWorld, theConfiguration);
	Log(LogLevel::Info) << "<> Imported " << countries.size() << " countries, dropped " << droppedLocations << " contested locations.";
	Log(LogLevel::Info) << "<> " << countries.size() - capitalFallbacks << "/" << countries.size()
							  << " capitals honor the CK3 realm capital; the rest fell back to the first owned location.";
	if (techFloorRaises > 0)
		Log(LogLevel::Info) << "<> " << techFloorRaises << " countries were behind what their setup template's laws require and had their technology raised to match.";
}

void EU5::World::loadEU5GameData(const std::filesystem::path& eu5Path)
{
	Log(LogLevel::Info) << "-> Loading EU5 location definitions.";
	locationDefinitions.loadDefinitions(eu5Path / "game" / "in_game" / "map_data" / "definitions.txt");
	locationDefinitions.loadPorts(eu5Path / "game" / "in_game" / "map_data" / "ports.csv");
	Log(LogLevel::Info) << "<> " << locationDefinitions.getLocationCount() << " locations found.";

	Log(LogLevel::Info) << "-> Loading EU5 cultures, religions and languages.";
	gameDatabase.loadCultures(eu5Path / "game" / "in_game" / "common" / "cultures");
	gameDatabase.loadReligions(eu5Path / "game" / "in_game" / "common" / "religions");
	gameDatabase.loadLanguages(eu5Path / "game" / "in_game" / "common" / "languages");
	Log(LogLevel::Info) << "<> " << gameDatabase.getCultureCount() << " cultures, " << gameDatabase.getReligionCount() << " religions, "
							  << gameDatabase.getLanguageCount() << " languages found.";

	Log(LogLevel::Info) << "-> Loading EU5 setup templates.";
	gameDatabase.loadSetupTemplates(eu5Path / "game" / "main_menu" / "setup" / "templates");
	Log(LogLevel::Info) << "<> " << gameDatabase.getTemplateCount() << " government templates found.";

	advances.loadAdvances(eu5Path / "game" / "in_game" / "common" / "advances");
	advances.loadPrivileges(eu5Path / "game" / "in_game" / "common" / "estate_privileges");
	Log(LogLevel::Info) << "<> " << advances.getAdvanceCount() << " advances read; laws and privileges now know what technology they need.";

	gameDatabase.loadCharacterNames(eu5Path / "game" / "main_menu" / "localization" / "english" / "character_names_dynamic_l_english.yml");

	Log(LogLevel::Info) << "-> Loading EU5 vanilla pops.";
	vanillaPops.loadPops(eu5Path / "game" / "main_menu" / "setup" / "start" / "06_pops.txt");
	Log(LogLevel::Info) << "<> Pops loaded for " << vanillaPops.getLocationCount() << " locations.";

	vanillaTowns.loadTowns(eu5Path / "game" / "main_menu" / "setup" / "start" / "07_cities_and_buildings.txt");
	Log(LogLevel::Info) << "<> " << vanillaTowns.getTownCount() << " vanilla towns and cities found.";
}

void EU5::World::loadMappers()
{
	Log(LogLevel::Info) << "-> Loading mappers.";
	provinceMapper.loadProvinceMappings("configurables/province_mappings.txt");
	tagMapper.loadMappings("configurables/tag_mappings.txt");
	religionMapper.loadMappings("configurables/religion_map.txt");
	cultureMapper.loadCultureMappingRules("configurables/culture_map.txt");
	cultureMapper.loadCultureGroupsMappingRules("configurables/cultureGroups_map.txt");
	cultureMapper.loadLanguageMappingRules("configurables/language_map.txt");
	governmentMapper.loadMappings("configurables/government_map.txt");
	traitMapper.loadMappings("configurables/trait_map.txt");
	lawMapper.loadMappings("configurables/law_map.txt");
	unitMapper.loadMappings("configurables/unit_map.txt");
	menAtArmsMapper.loadMappings("configurables/maa_map.txt");
	subjectTypeMapper.loadMappings("configurables/subject_map.txt");
	buildingMapper.loadMappings("configurables/building_map.txt");
	devWeights.loadWeights("configurables/dev_weights.txt");
	Log(LogLevel::Info) << "<> Mappers loaded.";
}

void EU5::World::importCountries(const CK3::World& sourceWorld, const configuration::Configuration& theConfiguration)
{
	Log(LogLevel::Info) << "-> Importing countries from independent CK3 realms.";
	// Larger realms get first pick of contested locations.
	std::vector<std::pair<std::string, std::shared_ptr<CK3::Title>>> sortedIndeps(sourceWorld.getIndeps().begin(), sourceWorld.getIndeps().end());
	std::ranges::sort(sortedIndeps, [](const auto& a, const auto& b) {
		return a.second->getOwnedDFCounties().size() > b.second->getOwnedDFCounties().size();
	});

	const auto shatterEmpires = theConfiguration.GetShatterEmpires();
	const auto vassalSplitoff = theConfiguration.GetVassalSplitoff();
	for (const auto& [name, title]: sortedIndeps)
	{
		const auto isImperial = title->getLevel() == CK3::LEVEL::EMPIRE || title->getLevel() == CK3::LEVEL::HEGEMONY;
		if (isImperial && (shatterEmpires || vassalSplitoff))
		{
			// Kingdom-tier vassals split off first and claim their land; the liege keeps whatever is
			// left, which for a shattered empire is the emperor's own demesne.
			std::vector<std::pair<std::string, std::string>> subjects; // tag -> EU5 subject type
			for (const auto& vassal: splitoffCandidates(*title))
			{
				const auto vassalTag = importCountry(vassal->getName(), vassal, sourceWorld);
				if (!vassalTag)
					continue;
				// These are internal feudal vassals, so the tributary contract groups don't apply: a
				// cadet branch crowned under the emperor is a fiefdom, anyone else a plain vassal.
				const auto dynastic = shareDynasty(title->getHolder()->second, vassal->getHolder()->second);
				subjects.emplace_back(*vassalTag, dynastic ? subjectTypeMapper.getDynasticType() : "vassal");
			}
			const auto liegeTag = importCountry(name, title, sourceWorld);
			if (!shatterEmpires && liegeTag)
				for (const auto& [vassalTag, subjectType]: subjects)
					if (vassalTag != *liegeTag)
						dependencies.push_back({*liegeTag, vassalTag, subjectType});
			continue;
		}
		// Realms convert whole: every independent CK3 realm becomes exactly one EU5 country.
		importCountry(name, title, sourceWorld);
	}

	classifyLandControl();
	applyUrbanQuota();
	importClaims(sourceWorld);
	importTributaries(sourceWorld);
	importAlliances(sourceWorld);
	importRivalries(sourceWorld);
	importWars(sourceWorld);
	importArtifacts(sourceWorld);
	importArmies(sourceWorld);
	importConfederations(sourceWorld);
	// The dynastic subject type (fiefdom) is only valid under a monarchy; a republic or horde
	// holding a cadet branch keeps it as a plain vassal instead of tripping game-start validation.
	for (auto& dependency: dependencies)
	{
		if (dependency.type != subjectTypeMapper.getDynasticType())
			continue;
		const auto overlord = countries.find(dependency.overlord);
		if (overlord == countries.end() || overlord->second.governmentType != "monarchy")
			dependency.type = "vassal";
	}
	Log(LogLevel::Info) << "<> " << dependencies.size() << " dependencies, " << alliances.size() << " alliances, " << rivalries.size() << " rivalries, "
							  << wars.size() << " wars imported.";
	if (hreTag)
		Log(LogLevel::Info) << "<> The Holy Roman Empire survives as " << *hreTag << ".";
	if (papacyTag)
		Log(LogLevel::Info) << "<> The Papacy survives as " << *papacyTag << " and leads the Catholic Church.";
}

void EU5::World::importClaims(const CK3::World& sourceWorld)
{
	// CK3 claims the ruler holds on foreign titles become EU5 cores on foreign land
	// (our_cores_conquered_by_others), giving converted realms their reconquest ambitions -
	// crucially without granting ownership. Only county and duchy claims convert -
	// kingdom and empire claims are too sweeping and would blanket half the map in cores.
	auto claimedLocations = 0;
	for (auto& [tag, country]: countries)
	{
		if (country.ck3Title.empty())
			continue;
		const auto& indeps = sourceWorld.getIndeps();
		const auto title = indeps.find(country.ck3Title);
		if (title == indeps.end() || !title->second->isHolderLinked())
			continue;
		const auto& holder = title->second->getHolder()->second;
		std::set<std::string> owned(country.locations.begin(), country.locations.end());
		for (const auto& [claimID, claimedTitle]: holder->getClaims())
		{
			if (!claimedTitle || claimedTitle->getLevel() >= CK3::LEVEL::KINGDOM)
				continue;
			for (const auto& [countyName, county]: claimedTitle->coalesceDJCounties())
			{
				if (!county)
					continue;
				for (const auto& [baronyID, barony]: county->getDJVassals())
				{
					if (!barony || !barony->getClay() || !barony->getClay()->getProvince())
						continue;
					for (const auto& location: provinceMapper.getEU5Locations(barony->getClay()->getProvince()->first))
					{
						if (!locationDefinitions.isValidLocation(location) || owned.contains(location))
							continue;
						if (country.coreClaims.insert(location).second)
							++claimedLocations;
					}
				}
			}
		}
	}
	Log(LogLevel::Info) << "<> " << claimedLocations << " locations claimed as cores from CK3 claims.";
}

void EU5::World::importTributaries(const CK3::World& sourceWorld)
{
	// CK3 tributaries hang off the character: the tributary ruler's suzerain points at the overlord.
	auto count = 0;
	for (auto& [tag, country]: countries)
	{
		if (country.ck3Title.empty())
			continue;
		const auto& indeps = sourceWorld.getIndeps();
		const auto title = indeps.find(country.ck3Title);
		if (title == indeps.end() || !title->second->isHolderLinked())
			continue;
		const auto& holder = title->second->getHolder()->second;
		if (!holder->getSuzerain() || !holder->getSuzerain()->second)
			continue;
		const auto overlord = rulerTags.find(holder->getSuzerain()->first);
		if (overlord == rulerTags.end() || overlord->second == tag)
			continue;

		// A cadet branch paying its senior line is family business, not tribute, whatever the
		// contract says. Otherwise the contract group decides how tightly the subject is held.
		const auto& contractGroup = sourceWorld.getVassalContracts().getContractGroup(holder->getID());
		const auto subjectType = shareDynasty(holder, holder->getSuzerain()->second) ? subjectTypeMapper.getDynasticType()
																												: subjectTypeMapper.getSubjectType(contractGroup);
		dependencies.push_back({overlord->second, tag, subjectType});
		++count;
	}
	if (count > 0)
		Log(LogLevel::Info) << "<> " << count << " tributaries imported.";
}

void EU5::World::importAlliances(const CK3::World& sourceWorld)
{
	std::set<std::pair<std::string, std::string>> seen;
	for (const auto& [firstID, secondID]: sourceWorld.getAlliancePairs())
	{
		const auto firstTag = rulerTags.find(firstID);
		const auto secondTag = rulerTags.find(secondID);
		if (firstTag == rulerTags.end() || secondTag == rulerTags.end())
			continue; // Only alliances between rulers of imported countries carry over.
		if (firstTag->second == secondTag->second)
			continue;
		const std::pair<std::string, std::string> pair = std::minmax(firstTag->second, secondTag->second);
		if (seen.contains(pair))
			continue;
		// Overlord-subject pairs are already bound tighter than any alliance.
		if (std::ranges::any_of(dependencies, [&pair](const auto& dependency) {
				 return (dependency.overlord == pair.first && dependency.subject == pair.second) ||
						  (dependency.overlord == pair.second && dependency.subject == pair.first);
			 }))
			continue;
		seen.insert(pair);
		alliances.push_back(pair);
	}
}

void EU5::World::importRivalries(const CK3::World& sourceWorld)
{
	std::set<std::pair<std::string, std::string>> seen;
	for (const auto& [ownerID, targetID]: sourceWorld.getRivalPairs())
	{
		const auto firstTag = rulerTags.find(ownerID);
		const auto secondTag = rulerTags.find(targetID);
		if (firstTag == rulerTags.end() || secondTag == rulerTags.end())
			continue;
		if (firstTag->second == secondTag->second)
			continue;
		const std::pair<std::string, std::string> pair = std::minmax(firstTag->second, secondTag->second);
		if (seen.contains(pair))
			continue;
		// Allies and overlord-subject pairs don't also get to be rivals.
		if (std::ranges::any_of(alliances, [&pair](const auto& alliance) {
				 return alliance == pair;
			 }))
			continue;
		if (std::ranges::any_of(dependencies, [&pair](const auto& dependency) {
				 return (dependency.overlord == pair.first && dependency.subject == pair.second) ||
						  (dependency.overlord == pair.second && dependency.subject == pair.first);
			 }))
			continue;
		seen.insert(pair);
		rivalries.push_back(pair);
	}
}

void EU5::World::importWars(const CK3::World& sourceWorld)
{
	if (!importActiveWars)
	{
		Log(LogLevel::Info) << "<> Active wars stay in CK3 by request; everyone starts at peace.";
		return;
	}
	// Targeted titles arrive as save IDs; the titles registry is keyed by name, so index it once.
	std::map<long long, std::shared_ptr<CK3::Title>> titlesByID;
	for (const auto& title: sourceWorld.getTitles().getTitles() | std::views::values)
		if (title)
			titlesByID.emplace(title->getID(), title);

	auto warIndex = 0;
	for (const auto& war: sourceWorld.getWars())
	{
		const auto attackerTag = rulerTags.find(war.attacker);
		const auto defenderTag = rulerTags.find(war.defender);
		// Internal wars (revolts, civil wars) and wars against unconverted rulers stay behind.
		if (attackerTag == rulerTags.end() || defenderTag == rulerTags.end() || attackerTag->second == defenderTag->second)
			continue;

		ConvertedWar converted;
		converted.rawName = war.name.empty() ? "War of Conquest" : resolveNestedName(war.name, sourceWorld.getLocalizationMapper());
		converted.nameKey = "conv_war_" + std::to_string(++warIndex);
		converted.startDate = war.startDate;
		converted.startDate.ChangeByYears(yearOffset);
		if (converted.startDate > date("1337.4.1"))
			converted.startDate = date("1337.1.1");

		const auto gatherSide = [this](const std::vector<long long>& participants, const std::string& primaryTag) {
			std::vector<std::string> tags{primaryTag};
			for (const auto participant: participants)
			{
				const auto participantTag = rulerTags.find(participant);
				if (participantTag == rulerTags.end())
					continue;
				if (std::ranges::find(tags, participantTag->second) == tags.end())
					tags.push_back(participantTag->second);
			}
			return tags;
		};
		converted.attackers = gatherSide(war.attackerParticipants, attackerTag->second);
		converted.defenders = gatherSide(war.defenderParticipants, defenderTag->second);
		// A tag can't fight itself; drop overlaps from the defenders.
		std::erase_if(converted.defenders, [&converted](const std::string& tag) {
			return std::ranges::find(converted.attackers, tag) != converted.attackers.end();
		});
		if (converted.defenders.empty())
			continue;

		// The wargoal: land of the CK3 casus belli's targeted title that a defender actually owns.
		// Wars without usable targeted titles are fought over the primary defender's seat instead.
		std::set<std::string> defenderLand;
		for (const auto& tag: converted.defenders)
			if (const auto& defender = countries.find(tag); defender != countries.end())
				defenderLand.insert(defender->second.locations.begin(), defender->second.locations.end());
		for (const auto titleID: war.targetedTitles)
		{
			const auto targeted = titlesByID.find(titleID);
			if (targeted == titlesByID.end() || !targeted->second)
				continue;
			for (const auto& [countyName, county]: targeted->second->coalesceDJCounties())
			{
				if (!county)
					continue;
				for (const auto& [baronyID, barony]: county->getDJVassals())
				{
					if (!barony || !barony->getClay() || !barony->getClay()->getProvince())
						continue;
					for (const auto& location: provinceMapper.getEU5Locations(barony->getClay()->getProvince()->first))
						if (defenderLand.contains(location))
						{
							converted.goalLocation = location;
							break;
						}
					if (!converted.goalLocation.empty())
						break;
				}
				if (!converted.goalLocation.empty())
					break;
			}
			if (!converted.goalLocation.empty())
				break;
		}
		if (converted.goalLocation.empty() && !defenderLand.empty())
		{
			const auto& primaryDefender = countries.find(converted.defenders.front());
			if (primaryDefender != countries.end() && defenderLand.contains(primaryDefender->second.capital))
				converted.goalLocation = primaryDefender->second.capital;
			else
				converted.goalLocation = *defenderLand.begin();
		}
		wars.push_back(converted);
	}
}

void EU5::World::importArtifacts(const CK3::World& sourceWorld)
{
	constexpr size_t maxPerCountry = 10;
	auto imported = 0;
	for (const auto& artifact: sourceWorld.getArtifacts())
	{
		const auto ownerTag = rulerTags.find(artifact.owner);
		if (ownerTag == rulerTags.end())
			continue;
		auto& country = countries.at(ownerTag->second);
		if (country.artworks.size() >= maxPerCountry)
			continue;
		ConvertedArtwork artwork;
		artwork.artType = artTypeForVisual(artifact.visualType);
		artwork.key = "conv_art_" + std::to_string(artifact.ID);
		artwork.rawName = artifact.name;
		artwork.rawDescription = artifact.description.empty() ? describeArtifact(artifact) : artifact.description;
		artwork.location = country.capital;
		artwork.quality = std::clamp(std::max(artifact.quality, artifact.wealth), 10, 90);
		// Works of art can't be created after the game starts, and an artifact made in the save's
		// final years lands past 1337 once shifted, so those settle on the eve of the bookmark.
		artwork.creationDate = date("1336.1.1");
		if (artifact.creationDate)
		{
			auto made = *artifact.creationDate;
			made.ChangeByYears(yearOffset);
			if (made < artwork.creationDate)
				artwork.creationDate = made;
		}
		country.artworks.push_back(artwork);
		++imported;
	}
	if (imported > 0)
		Log(LogLevel::Info) << "<> " << imported << " artifacts imported as works of art.";
}

void EU5::World::importConfederations(const CK3::World& sourceWorld)
{
	// CK3 house blocs - steppe confederations, Japanese clan alliances - are standing associations of
	// dynasties rather than of realms, so each becomes an EU5 tribal confederation of whichever
	// converted countries those houses ended up ruling. A bloc down to one surviving realm is no
	// longer a confederation.
	for (const auto& confederation: sourceWorld.getConfederations().getConfederations() | std::views::values)
	{
		std::vector<std::string> members;
		for (const auto& house: confederation->getHouses())
			if (const auto& tag = houseTags.find(house); tag != houseTags.end())
				members.push_back(tag->second);
		std::ranges::sort(members);
		members.erase(std::ranges::unique(members).begin(), members.end());
		if (members.size() < 2)
			continue;
		confederations.emplace_back(confederation->getName(), members);
	}
	if (!confederations.empty())
		Log(LogLevel::Info) << "<> " << confederations.size() << " house confederations carried over as tribal confederations.";
}

std::optional<long long> EU5::World::findDeJureClaimant(const CK3::Title& county)
{
	// Whoever wears the crown this county de jure belongs to. If that is somebody other than the
	// realm sitting on the county, the land is disputed rather than merely foreign.
	auto liege = county.getDJLiege();
	while (liege && liege->second && liege->second->getLevel() < CK3::LEVEL::KINGDOM)
		liege = liege->second->getDJLiege();
	if (!liege || !liege->second || !liege->second->isHolderLinked())
		return std::nullopt;
	return liege->second->getHolder()->first;
}

void EU5::World::classifyLandControl()
{
	// EU5 splits a realm's land three ways, and the split decides how much of it the country can
	// actually use. Cored land pays full tax and answers fully; integrated land is firmly ruled but
	// governed as a distinct possession; conquered land is a fresh wound of unrest and lost revenue.
	//
	// Vanilla reserves "conquered" for land taken within a few years of the start date - only 2% of
	// the 1337 map - and uses "integrated" for the long-held foreign holdings: the Golden Horde's
	// Rus principalities, Delhi's Deccan, France's appanages, Venice's Aegean islands. A CK3 realm
	// that spent three centuries absorbing its neighbours has earned the same treatment, so land
	// only converts as conquered while a rival still wears its de jure crown.
	auto coreCount = 0;
	auto integratedCount = 0;
	auto conqueredCount = 0;
	for (auto& country: countries | std::views::values)
	{
		for (const auto& location: country.locations)
		{
			if (!country.nonDeJureLocations.contains(location))
			{
				++coreCount; // the realm's own de jure homeland
				continue;
			}
			// Land the realm has culturally absorbed is homeland too, whatever the de jure map says:
			// a century of shared culture and faith outlasts any title's paper claim.
			const auto& detail = locationDetails.find(location);
			const auto assimilated = detail != locationDetails.end() && gameDatabase.sharesCultureGroup(detail->second.culture, country.culture) &&
											 (detail->second.religion == country.religion ||
												  gameDatabase.getReligionGroup(detail->second.religion) == gameDatabase.getReligionGroup(country.religion));
			if (assimilated)
			{
				++coreCount;
				continue;
			}
			const auto& dispute = country.disputedLocations.find(location);
			const auto& claimantTag = dispute != country.disputedLocations.end() ? rulerTags.find(dispute->second) : rulerTags.end();
			if (claimantTag != rulerTags.end() && claimantTag->second != country.tag)
			{
				country.conqueredLocations.insert(location);
				++conqueredCount;
			}
			else
			{
				country.integratedLocations.insert(location);
				++integratedCount;
			}
		}
	}
	const auto total = std::max(1, coreCount + integratedCount + conqueredCount);
	Log(LogLevel::Info) << "<> Land control: " << coreCount << " cored, " << integratedCount << " integrated, " << conqueredCount << " freshly conquered ("
							  << 100 * coreCount / total << "/" << 100 * integratedCount / total << "/" << 100 * conqueredCount / total << "%).";
}

void EU5::World::applyUrbanQuota()
{
	// CK3 scatters city holdings far more freely than EU5 places towns, and a converted save that
	// honoured every one of them would hand the world two and a half times vanilla's urban footprint.
	// Each town carries a setup worth about fifteen buildings that pops have to staff and goods have
	// to feed, so an over-urbanised map is one where nothing turns a profit. Rather than judge
	// counties against an absolute development number - which drifts upward for the whole map as a
	// campaign matures - the candidates compete against each other for a fixed number of slots,
	// measured against vanilla's own urban density on the very land being converted.
	auto vanillaUrban = 0;
	auto vanillaCities = 0;
	for (const auto& location: locationDetails | std::views::keys)
	{
		if (!vanillaTowns.isUrban(location))
			continue;
		++vanillaUrban;
		if (vanillaTowns.isCity(location))
			++vanillaCities;
	}
	const auto urbanBudget = static_cast<int>(vanillaUrban * devWeights.getUrbanDensityAllowance());
	const auto newTownBudget = std::max(0, urbanBudget - vanillaUrban);
	const auto cityBudget = static_cast<int>(urbanBudget * devWeights.getCityShareOfUrban());

	// Best developed first, so the CK3 cities a player actually built up are the ones that survive
	// the cut. Location name breaks ties to keep the outcome stable between runs.
	std::vector<std::string> candidates;
	for (const auto& [location, detail]: locationDetails)
		if (detail.cityHolding)
			candidates.push_back(location);
	std::ranges::sort(candidates, [this](const std::string& first, const std::string& second) {
		const auto firstDev = locationDetails.at(first).development;
		const auto secondDev = locationDetails.at(second).development;
		return firstDev != secondDev ? firstDev > secondDev : first < second;
	});

	auto founded = 0;
	auto promoted = 0;
	for (const auto& location: candidates)
	{
		auto& detail = locationDetails.at(location);
		// A location vanilla already made urban stays urban and costs nothing from the budget; the
		// rest are new towns this conversion is asking the world's economy to support.
		if (vanillaTowns.isUrban(location))
			detail.town = true;
		else if (founded < newTownBudget)
		{
			detail.town = true;
			++founded;
		}
		if (!detail.town || vanillaTowns.isCity(location))
			continue;
		// City rank goes to the most developed of the urban locations that don't already hold it,
		// whether that means founding one outright or promoting a vanilla town.
		if (vanillaCities + promoted < cityBudget)
		{
			detail.city = true;
			++promoted;
		}
	}
	Log(LogLevel::Info) << "<> Urban quota: " << candidates.size() << " CK3 city holdings competed for " << newTownBudget << " new town slots (vanilla holds "
							  << vanillaUrban << " urban locations on this land); " << founded << " founded, " << promoted << " granted city rank.";
}

void EU5::World::importArmies(const CK3::World& sourceWorld)
{
	// The men-at-arms rulers actually maintained in CK3 become EU5 standing armies. Regiments
	// belong to individual characters, so a realm's army is the ruler's plus every vassal's:
	// vassals merged into the realm when it converted, and so do their soldiers. Levies are not
	// imported - EU5 raises those from population natively.
	const auto& characters = sourceWorld.getCharacters().getCharacters();
	auto importedMen = 0;
	auto orphanedMen = 0;
	for (const auto& [ownerID, regiments]: sourceWorld.getMenAtArms())
	{
		// Attribute the owner to a converted country: independent rulers directly, vassals by
		// walking their primary title's de facto lieges up to the independent top.
		auto tagMatch = rulerTags.find(ownerID);
		if (tagMatch == rulerTags.end())
		{
			const auto character = characters.find(ownerID);
			if (character == characters.end() || !character->second || !character->second->getCharacterDomain() ||
				 character->second->getCharacterDomain()->getDomain().empty())
			{
				for (const auto& [maaType, men]: regiments)
					orphanedMen += men;
				continue;
			}
			auto liegeTitle = character->second->getCharacterDomain()->getDomain().front().second;
			while (liegeTitle && liegeTitle->getDFLiege() && liegeTitle->getDFLiege()->second)
				liegeTitle = liegeTitle->getDFLiege()->second;
			if (liegeTitle && liegeTitle->isHolderLinked())
				tagMatch = rulerTags.find(liegeTitle->getHolder()->first);
		}
		if (tagMatch == rulerTags.end())
		{
			for (const auto& [maaType, men]: regiments)
				orphanedMen += men;
			continue;
		}
		auto& country = countries.at(tagMatch->second);
		for (const auto& [maaType, men]: regiments)
		{
			if (const auto unit = menAtArmsMapper.getUnitForMAA(maaType))
			{
				country.maaUnits[*unit] += men;
				importedMen += men;
			}
		}
	}
	Log(LogLevel::Info) << "<> " << importedMen << " men-at-arms imported into standing armies, " << orphanedMen
							  << " belonged to unconverted characters and disbanded.";
}

std::optional<std::string> EU5::World::importCountry(const std::string& ck3TitleName, const std::shared_ptr<CK3::Title>& title, const CK3::World& sourceWorld)
{
	if (!title || !title->isHolderLinked())
		return std::nullopt;
	const auto& holder = title->getHolder()->second;

	Country country;
	country.ck3Title = ck3TitleName;

	// The realm's de jure counties. Land held outside them is a possession rather than homeland,
	// though how firmly the realm holds it only gets decided in classifyLandControl.
	std::set<std::string> deJureCounties;
	for (const auto& [countyName, county]: title->getOwnedDJCounties())
		deJureCounties.insert(countyName);
	if (holder->getCharacterDomain())
		for (const auto& [domainTitleID, domainTitle]: holder->getCharacterDomain()->getDomain())
			if (domainTitle)
				for (const auto& [countyName, county]: domainTitle->getOwnedDJCounties())
					deJureCounties.insert(countyName);

	// Gather owned locations from all defacto counties.
	for (const auto& [countyName, county]: title->getOwnedDFCounties())
	{
		if (!county)
			continue;
		const auto outsideDeJure = !deJureCounties.contains(countyName);
		const auto claimant = findDeJureClaimant(*county);
		for (const auto& location: getLocationsForCounty(*county))
		{
			country.locations.push_back(location);
			if (outsideDeJure)
				country.nonDeJureLocations.insert(location);
			if (claimant)
				country.disputedLocations.emplace(location, *claimant);
		}
	}
	if (country.locations.empty())
		return std::nullopt; // Landless or fully contested realms don't make it onto the map.

	// A ruler already on the map holds multiple crowns; the extra crown is annexed into the
	// primary country instead of spawning a second country with the same ruler (personal union resolution).
	if (const auto& existingTag = rulerTags.find(holder->getID()); existingTag != rulerTags.end())
	{
		auto& primary = countries.at(existingTag->second);
		for (const auto& location: country.locations)
		{
			primary.locations.push_back(location);
			primary.nonDeJureLocations.insert(location); // a second crown is not the primary crown's de jure land
		}
		Log(LogLevel::Info) << "<> " << ck3TitleName << " is in personal union under " << primary.ck3Title << "; annexed into " << existingTag->second << ".";
		return existingTag->second;
	}

	// Capital: the holder's realm capital barony, widening to its whole county if the barony's own
	// location was contested away, and only then falling back to the first owned location.
	if (holder->getCharacterDomain() && holder->getCharacterDomain()->getRealmCapital().second)
	{
		const auto& capitalBarony = holder->getCharacterDomain()->getRealmCapital().second;
		const auto ownsLocation = [&country](const std::string& location) {
			return std::ranges::find(country.locations, location) != country.locations.end();
		};
		if (capitalBarony->getClay() && capitalBarony->getClay()->getProvince())
		{
			for (const auto& location: provinceMapper.getEU5Locations(capitalBarony->getClay()->getProvince()->first))
			{
				if (ownsLocation(location))
				{
					country.capital = location;
					break;
				}
			}
		}
		if (country.capital.empty() && capitalBarony->getDJLiege() && capitalBarony->getDJLiege()->second)
		{
			for (const auto& [siblingID, sibling]: capitalBarony->getDJLiege()->second->getDJVassals())
			{
				if (!sibling || !sibling->getClay() || !sibling->getClay()->getProvince())
					continue;
				for (const auto& location: provinceMapper.getEU5Locations(sibling->getClay()->getProvince()->first))
				{
					if (ownsLocation(location))
					{
						country.capital = location;
						break;
					}
				}
				if (!country.capital.empty())
					break;
			}
		}
	}
	if (country.capital.empty())
	{
		country.capital = country.locations.front();
		++capitalFallbacks;
	}

	// Tag, preferring capital-based mappings.
	const auto tag = tagMapper.getTagForTitle(ck3TitleName, country.capital);
	if (!tag)
		return std::nullopt;
	country.tag = *tag;

	country.displayName = resolveDisplayName(sourceWorld, *title);
	// The player's realm displays under its CK3 UI name (dynamic titles often carry stale internal names).
	if (sourceWorld.getPlayerTitle() && *sourceWorld.getPlayerTitle() == ck3TitleName && sourceWorld.getMetaTitleName())
	{
		auto metaName = *sourceWorld.getMetaTitleName();
		if (metaName.starts_with("the ") || metaName.starts_with("The "))
			metaName = metaName.substr(4);
		if (!metaName.empty())
			country.displayName = metaName;
	}
	country.adjective = resolveAdjective(sourceWorld, *title, country.displayName);
	country.color = title->getColor();
	if (title->getCoA() && title->getCoA()->second)
		country.coa = title->getCoA()->second;
	// Dynamic realm titles (nomads, adventurers - x_ prefixed) display their ruling dynasty's arms
	// in CK3, not the internal title arms, which are often stale leftovers.
	if (ck3TitleName.starts_with("x_") && holder->getHouse().second)
	{
		const auto& house = holder->getHouse().second;
		if (house->getDynasty().second && house->getDynasty().second->getCoA() && house->getDynasty().second->getCoA()->second)
			country.coa = house->getDynasty().second->getCoA()->second;
	}
	// The player realm's displayed arms are stored verbatim in the save metadata - the best source there is.
	if (sourceWorld.getPlayerTitle() && *sourceWorld.getPlayerTitle() == ck3TitleName && sourceWorld.getMetaCoA())
		country.coa = sourceWorld.getMetaCoA();

	switch (title->getLevel())
	{
		case CK3::LEVEL::EMPIRE:
		case CK3::LEVEL::HEGEMONY:
			country.rank = "rank_empire";
			break;
		case CK3::LEVEL::KINGDOM:
			country.rank = "rank_kingdom";
			break;
		case CK3::LEVEL::DUCHY:
			country.rank = "rank_duchy";
			break;
		default:
			country.rank = "rank_county";
	}

	country.religion = determineReligion(holder, country.capital);
	country.culture = determineCulture(holder, country.capital);

	// Government: category from the CK3 government string, everything else from the government map,
	// so the written type/parliament always match the included template.
	const auto category = determineGovernmentCategory(holder);
	const auto religionGroup = gameDatabase.getReligionGroup(country.religion);
	// Generated religions have no group of their own; they inherit their parent's for this purpose.
	auto templateReligion = country.religion;
	auto templateGroup = religionGroup;
	if (const auto& generated = generatedReligions.find(country.religion); generated != generatedReligions.end())
	{
		templateReligion.clear(); // no vanilla religion to match exactly
		templateGroup = generated->second.group;
	}
	if (const auto government = governmentMapper.getGovernment(category, templateReligion, templateGroup))
	{
		country.governmentType = government->governmentType;
		country.templateInclude = government->setupTemplate;
		country.parliamentType = government->parliament;
		country.heirSelection = government->heirSelection;
		country.techLevel = government->techLevel;
	}
	else
	{
		Log(LogLevel::Warning) << "No government mapping for " << category << "/" << templateGroup << "; " << ck3TitleName << " defaults to a monarchy.";
		country.governmentType = "monarchy";
		country.templateInclude = "subsaharan_monarchy";
		country.parliamentType = "assembly";
		country.heirSelection = "cognatic_primogeniture";
	}
	country.unitCategory = category == "tribe" ? "tribal" : category == "horde" ? "horde" : "feudal";
	// Vanilla ships a landlocked twin of most templates, because the coastal ones hand out privileges
	// like sponsoring maritime contracts that an inland realm can never satisfy.
	if (!country.templateInclude.empty())
	{
		const auto coastal = std::ranges::any_of(country.locations, [this](const std::string& location) {
			return !locationDefinitions.getPortSeaZone(location).empty();
		});
		if (!coastal && gameDatabase.hasTemplate(country.templateInclude + "_no_coast"))
			country.templateInclude += "_no_coast";
	}

	// The theological school the faith belongs to, where EU5 tracks one.
	if (holder->getFaith() && holder->getFaith()->second)
	{
		const auto& faith = holder->getFaith()->second;
		auto mapping = religionMapper.getEU5ReligionForCK3Faith(faith->getName(), faith->getReligiousHead());
		if (!mapping || !mapping->school)
			mapping = religionMapper.getEU5ReligionForCK3Faith(faith->getTemplate(), std::string());
		if (mapping && mapping->school)
			country.religiousSchool = *mapping->school;
	}

	// Societal values: the template's own numbers are the starting point, realm laws overrule them
	// outright, and the culture's ethos and traditions nudge the result.
	country.societalValues = gameDatabase.getTemplateValues(country.templateInclude);
	const auto& laws = title->getLaws();
	for (const auto& [value, position]: lawMapper.getValuePositions(laws))
		country.societalValues[value] = position;
	if (holder->getCulture() && holder->getCulture()->second)
	{
		const auto& culture = holder->getCulture()->second;
		auto shifts = lawMapper.getEthosShifts(culture->getEthos());
		for (const auto& [value, shift]: lawMapper.getTraditionShifts(culture->getTraditions()))
			shifts[value] += shift;
		for (const auto& [value, shift]: shifts)
			country.societalValues[value] = std::clamp(country.societalValues[value] + shift, -100, 100);

		// Technology follows how far the ruling culture actually got in CK3. A people still in the
		// tribal era in the 1300s is genuinely behind; one in the high medieval era matches the
		// level vanilla gives settled 1337 states.
		// With ck3TechLevels off the setup template's own level stands instead.
		if (const auto& era = culture->getEra(); ck3TechLevels && !era.empty())
		{
			if (era == "culture_era_tribal")
				country.techLevel = 1;
			else if (era == "culture_era_early_medieval")
				country.techLevel = 2;
			else
				country.techLevel = 3;
		}
	}
	// A setup template is a package: its laws and estate privileges assume the technology level it
	// was written for. Lowering a country below that leaves it holding laws no advance of its has
	// unlocked, which the game rejects one line at a time. Vanilla pairs its level-0 countries with
	// tribal templates that ask for nothing, so the honest fix is to let the CK3 era pull technology
	// down only as far as the template it came with can still stand.
	if (const auto& setups = gameDatabase.getTemplateSetups(); setups.contains(country.templateInclude))
	{
		const auto& setup = setups.at(country.templateInclude);
		auto floor = 0;
		for (const auto& law: setup.laws)
			floor = std::max(floor, advances.getLawTechLevel(law));
		for (const auto& privilege: setup.privileges)
			floor = std::max(floor, advances.getPrivilegeTechLevel(privilege));
		if (country.techLevel < floor)
		{
			country.techLevel = floor;
			++techFloorRaises;
		}
	}
	if (const auto heirSelection = lawMapper.getHeirSelection(laws))
		country.heirSelection = *heirSelection;

	// Accepted and tolerated cultures: whoever else lives on this realm's land. Without them a
	// converted empire treats nine tenths of its own population as foreign.
	{
		std::map<std::string, int> cultureShares;
		for (const auto& location: country.locations)
			if (const auto& detail = locationDetails.find(location); detail != locationDetails.end() && !detail->second.culture.empty())
				++cultureShares[detail->second.culture];
		const auto total = static_cast<double>(country.locations.size());
		std::vector<std::pair<int, std::string>> ranked;
		for (const auto& [culture, count]: cultureShares)
			if (culture != country.culture)
				ranked.emplace_back(count, culture);
		std::ranges::sort(ranked, std::greater{});
		for (const auto& [count, culture]: ranked)
		{
			const auto share = total > 0 ? count / total : 0.0;
			// Acceptance is expensive - three times what tolerance costs - and EU5 caps how much of
			// it a government can afford, so vanilla spends it sparingly: 72 accepted cultures in
			// the whole 1337 world against some 1,269 tolerated ones, with tolerated lists running
			// dozens of entries long. Only a minority holding a fifth of the realm earns the crown's
			// full recognition, and only the largest such; everyone else is tolerated, however many
			// of them there are.
			if (share >= 0.2 && country.acceptedCultures.empty())
				country.acceptedCultures.push_back(culture);
			else
				country.toleratedCultures.push_back(culture);
		}
	}

	// Court language follows the primary culture; liturgical language follows the faith. Both are
	// resolved to leaf languages - the game rejects references to dialect-bearing parents.
	if (const auto generatedCulture = generatedCultures.find(country.culture); generatedCulture != generatedCultures.end())
		country.courtLanguage = gameDatabase.resolveLanguage(generatedCulture->second.language);
	else
		country.courtLanguage = gameDatabase.resolveLanguage(gameDatabase.getCultureLanguage(country.culture));
	if (const auto generatedReligion = generatedReligions.find(country.religion); generatedReligion != generatedReligions.end())
		country.liturgicalLanguage = gameDatabase.resolveLanguage(generatedReligion->second.language);
	else
		country.liturgicalLanguage = gameDatabase.resolveLanguage(gameDatabase.getReligionLanguage(country.religion));

	// Ruler, family tree and dynasties.
	buildFamily(holder, country, sourceWorld);
	const auto findInFamily = [&country](long long ck3ID) -> ConvertedCharacter* {
		const auto key = "conv_char_" + std::to_string(ck3ID) + "_" + country.tag;
		for (auto& member: country.family)
			if (member.key == key)
				return &member;
		return nullptr;
	};
	if (auto* ruler = findInFamily(holder->getID()))
	{
		// The realm's religion and culture are derived from this very character; disagreements are
		// mapping artifacts (fallbacks taking different paths), so the ruler follows the realm.
		ruler->religion = country.religion;
		ruler->culture = country.culture;
		// The title's date field records the last succession, i.e. when this ruler took the crown.
		// EU5 grants rulers roughly one ruler trait per five years of reign and logs errors past
		// that, so the reign carries over and the traits are capped to what the reign justifies.
		if (title->getCreationDate() != date("1.1.1") && title->getCreationDate() <= sourceWorld.getConversionDate())
		{
			auto reignStart = title->getCreationDate();
			reignStart.ChangeByYears(yearOffset);
			country.reignStart = reignStart;
			const auto reignYears = std::max(0, 1337 - reignStart.getYear());
			if (ruler->rulerTraits.size() > static_cast<size_t>(reignYears / 5))
				ruler->rulerTraits.resize(reignYears / 5);
		}
		else
			ruler->rulerTraits.clear(); // no known reign start; the game expects zero traits
		// A prince-bishop answers to the church and a doge to the merchants, whatever their birth.
		if (country.governmentType == "theocracy")
			ruler->estate = "clergy_estate";
		else if (country.governmentType == "republic")
			ruler->estate = "burghers_estate";
		country.ruler = *ruler;
		// The ruler's personal CK3 gold arrives as starting treasury (1 gold ~ 1 ducat), capped
		// below Mansa Musa's vanilla 2500.
		country.treasury = importTreasury ? std::clamp(static_cast<int>(holder->getGold()), 0, 2000) : 0;
		if (!ruler->dynastyKey.empty())
			country.dynasty = dynasties.at(ruler->dynastyKey);

		// The line of rule behind the throne. CK3 records who held the title before, and most of
		// them are ancestors already exported with the family tree, so their reigns can be written
		// out as real ruler_terms: the dynasty arrives in EU5 having ruled for generations, and the
		// current monarch is numbered against the namesakes who came before.
		std::vector<std::pair<date, const CK3::Character*>> line; // death date -> holder
		for (const auto& [previousID, previous]: title->getPreviousHolders())
			if (previous && previous->getID() != holder->getID() && previous->getDeathDate())
				line.emplace_back(*previous->getDeathDate(), previous.get());
		std::ranges::sort(line, [](const auto& lhs, const auto& rhs) {
			return lhs.first < rhs.first;
		});
		std::map<std::string, int> namesakes; // first name -> reigns already counted under it
		std::optional<date> previousEnd;
		for (const auto& [death, previous]: line)
		{
			auto reignEnd = death;
			reignEnd.ChangeByYears(yearOffset);
			// A reign runs from the last one's end. The earliest we know of has to start somewhere,
			// so it starts when its holder came of age.
			auto reignStart = previousEnd.value_or([&] {
				auto comingOfAge = previous->getBirthDate();
				comingOfAge.ChangeByYears(yearOffset + 16);
				return comingOfAge;
			}());
			previousEnd = reignEnd;
			if (reignEnd <= reignStart)
				continue; // a reign the shifted dates can't place in order
			const auto number = ++namesakes[previous->getName()];
			if (const auto* ancestor = findInFamily(previous->getID()))
				country.pastReigns.push_back({ancestor->key, reignStart, reignEnd, number});
		}
		country.regnalNumber = 1;
		if (const auto& shared = namesakes.find(holder->getName()); shared != namesakes.end())
			country.regnalNumber = shared->second + 1;

		// Hand the tally to EU5 so the heir born after the conversion is crowned the next in line
		// rather than restarting at the first of his name. Names the game doesn't know are dropped.
		namesakes[holder->getName()] = country.regnalNumber;
		for (const auto& [name, count]: namesakes)
			if (const auto key = gameDatabase.getNameKey(name); !key.empty())
				country.regnalNames[key] = std::max(country.regnalNames[key], count);

		country.aiPersonality = determineAIPersonality(holder, country);
	}
	if (holder->getSpouse() && holder->getSpouse()->second && !holder->getSpouse()->second->isDead())
	{
		if (const auto* consort = findInFamily(holder->getSpouse()->first))
			country.consort = *consort;
	}
	for (const auto& [heirID, heir]: title->getHeirs())
	{
		if (!heir || heir->isDead())
			continue;
		if (const auto* member = findInFamily(heirID))
			country.heir = *member;
		else
		{
			// A distant heir outside the exported family; bring them in without links.
			auto converted = convertCharacter(heir, country, sourceWorld);
			converted.tag = country.tag;
			converted.dynastyKey = registerDynasty(heir, country, sourceWorld);
			country.family.push_back(converted);
			country.heir = converted;
		}
		break;
	}
	// A consort who is also the designated heir reads as nonsense in EU5; succession sorts itself out.
	if (country.heir && country.consort && country.heir->key == country.consort->key)
		country.heir.reset();

	buildCourt(holder, country, sourceWorld);

	if (ck3TitleName == "e_hre")
		hreTag = country.tag;
	// Whoever holds the Papacy in CK3 leads EU5's catholic_church, and their court is the Curia.
	if (title->isThePope() || ck3TitleName == "k_papal_state")
		papacyTag = country.tag;
	if (holder->getHouse().second)
		houseTags.emplace(holder->getHouse().first, country.tag);

	// No realm opens its books at nothing. CK3 rulers habitually sit near zero gold, having just
	// spent everything on a war or a cathedral, but a country starting at zero ducats has no runway:
	// control and tax efficiency both climb over the first years, so the opening months run lean by
	// design and the first unlucky one means a loan. Vanilla hands explicit gold to twenty-one
	// countries; here every realm gets a floor scaled to the land it has to administer, and anything
	// its ruler actually hoarded counts over that.
	country.treasury = std::max(country.treasury, std::min(500, 50 + 2 * static_cast<int>(country.locations.size())));

	rulerTags[holder->getID()] = country.tag;
	countries.emplace(country.tag, country);
	return country.tag;
}

std::vector<std::string> EU5::World::getLocationsForCounty(const CK3::Title& county)
{
	// County-wide data every mapped location inherits: culture, faith and development.
	std::shared_ptr<CK3::CountyDetail> detail;
	if (county.getClay() && county.getClay()->getCounty())
		detail = county.getClay()->getCounty()->second;

	std::vector<std::string> gatheredLocations;
	std::map<std::string, std::string> locationHoldings; // location -> ck3 holding type of the source barony
	std::map<std::string, int> locationBuildings;			// location -> building count in the source barony
	std::map<std::string, std::set<std::string>> locationEU5Buildings;
	for (const auto& [baronyID, barony]: county.getDJVassals())
	{
		if (!barony || !barony->getClay() || !barony->getClay()->getProvince())
			continue;
		std::string holdingType;
		auto buildingCount = 0;
		std::set<std::string> eu5Buildings;
		if (barony->getClay()->getProvince()->second)
		{
			const auto& holding = barony->getClay()->getProvince()->second;
			holdingType = holding->getHoldingType();
			buildingCount = holding->countBuildings();
			for (const auto& building: holding->getBuildings())
				if (const auto eu5Building = buildingMapper.getEU5BuildingForCK3Building(building))
					eu5Buildings.insert(*eu5Building);
			// Holy sites, famous universities and unique wonders are special buildings, kept apart
			// from the regular building list in the save.
			if (!holding->getSpecialBuilding().empty())
				if (const auto eu5Building = buildingMapper.getEU5BuildingForCK3Building(holding->getSpecialBuilding()))
					eu5Buildings.insert(*eu5Building);
		}
		// Player renames carry over: a renamed barony renames its location; a county rename lands
		// on the county capital's location ("Malik City" stays "Malik City", not "Palma").
		auto customName = barony->getCustomName();
		if (!customName && barony->isCountyCapitalBarony())
			customName = county.getCustomName();
		for (const auto& location: provinceMapper.getEU5Locations(barony->getClay()->getProvince()->first))
		{
			if (!locationDefinitions.isValidLocation(location))
				continue;
			if (takenLocations.contains(location))
			{
				++droppedLocations;
				continue;
			}
			takenLocations.insert(location);
			gatheredLocations.push_back(location);
			locationHoldings[location] = holdingType;
			locationBuildings[location] = buildingCount;
			// Only the first location gets the actual buildings: a sprawling multi-location barony
			// holds one Hagia Sophia, not one per location it happens to cover.
			locationEU5Buildings[location] = eu5Buildings;
			eu5Buildings.clear();
			if (customName)
			{
				// Only the first location a barony maps to takes the name; sprawling multi-location
				// baronies shouldn't produce five identical "Malik City"s.
				locationRenames[location] = *customName;
				customName.reset();
			}
		}
	}

	if (detail)
	{
		for (const auto& location: gatheredLocations)
		{
			LocationDetails details;
			details.development = detail->getDevelopment();
			details.buildings = locationBuildings[location];
			details.eu5Buildings = locationEU5Buildings[location];
			if (detail->getCulture().second)
				details.culture = convertCulture(detail->getCulture().second, location);
			if (detail->getFaith().second)
				details.religion = convertFaith(detail->getFaith().second, location);
			// A city holding only makes the location a candidate for urban rank; applyUrbanQuota
			// decides which candidates actually get one, once the whole map is known.
			details.cityHolding = locationHoldings[location] == "city_holding";
			locationDetails[location] = details;
		}
	}
	return gatheredLocations;
}

std::string EU5::World::resolveDisplayName(const CK3::World& sourceWorld, const CK3::Title& title) const
{
	if (!title.getDisplayName().empty())
		return title.getDisplayName();
	if (const auto& locBlock = sourceWorld.getLocalizationMapper().getLocBlockForKey(title.getName()); locBlock)
		return locBlock->english;
	return title.getName();
}

std::string EU5::World::resolveAdjective(const CK3::World& sourceWorld, const CK3::Title& title, const std::string& displayName) const
{
	// Landed titles carry real CK3 adjectives ("Byzantine" for e_byzantium). Renamed and dynamic
	// titles have no usable adjective loc, so derive one from the display name instead.
	if (title.getDisplayName().empty())
		if (const auto& locBlock = sourceWorld.getLocalizationMapper().getLocBlockForKey(title.getName() + "_adj"); locBlock && !locBlock->english.empty())
			return locBlock->english;
	return stripRankWords(displayName);
}

std::string EU5::World::determineReligion(const std::shared_ptr<CK3::Character>& holder, const std::string& capital)
{
	if (holder->getFaith() && holder->getFaith()->second)
		return convertFaith(holder->getFaith()->second, capital);
	if (const auto& dominant = vanillaPops.getDominantPop(capital); dominant && gameDatabase.isValidReligion(dominant->religion))
		return dominant->religion;
	return "catholic";
}

std::string EU5::World::determineCulture(const std::shared_ptr<CK3::Character>& holder, const std::string& capital)
{
	if (holder->getCulture() && holder->getCulture()->second)
		return convertCulture(holder->getCulture()->second, capital);
	if (const auto& dominant = vanillaPops.getDominantPop(capital); dominant && gameDatabase.isValidCulture(dominant->culture))
		return dominant->culture;
	return "swedish"; // Should never happen; better a wrong culture than a broken country.
}

bool EU5::World::isValidOrGeneratedReligion(const std::string& religion) const
{
	return gameDatabase.isValidReligion(religion) || generatedReligions.contains(religion);
}

bool EU5::World::isValidOrGeneratedCulture(const std::string& culture) const
{
	return gameDatabase.isValidCulture(culture) || generatedCultures.contains(culture);
}

std::string EU5::World::convertFaith(const std::shared_ptr<CK3::Faith>& faith, const std::string& location)
{
	if (faith)
	{
		// Already generated a religion for this faith? Keep using it.
		if (const auto& generated = generatedFaithNames.find(faith->getID()); generated != generatedFaithNames.end())
			return generated->second;

		const auto& mapping = religionMapper.getEU5ReligionForCK3Faith(faith->getName(), faith->getReligiousHead());
		const auto isCustom = dynamicReligions && (faith->getReformedFlag() || !faith->getCustomName().empty());
		if (!isCustom && mapping && gameDatabase.isValidReligion(mapping->eu5Religion))
			return mapping->eu5Religion;

		// Custom, reformed or unmapped faiths become real EU5 religions instead of collapsing
		// into the nearest vanilla one. Group and language borrow from the closest mapped parent.
		std::string parent;
		if (mapping && gameDatabase.isValidReligion(mapping->eu5Religion))
			parent = mapping->eu5Religion;
		else if (const auto& templateMapping = religionMapper.getEU5ReligionForCK3Faith(faith->getTemplate(), std::string());
					templateMapping && gameDatabase.isValidReligion(templateMapping->eu5Religion))
			parent = templateMapping->eu5Religion;
		else if (const auto& dominant = vanillaPops.getDominantPop(location); dominant && gameDatabase.isValidReligion(dominant->religion))
			parent = dominant->religion;
		else
			parent = "catholic";

		if (!dynamicReligions)
			return parent; // the player asked for vanilla religions only, so the faith folds into its closest parent

		GeneratedReligion generated;
		generated.name = "conv_rel_" + sanitizeKey(faith->getName());
		generated.rawName = !faith->getCustomName().empty() ? faith->getCustomName() : faith->getName();
		generated.group = gameDatabase.getReligionGroup(parent);
		generated.language = gameDatabase.resolveLanguage(gameDatabase.getReligionLanguage(parent));
		generated.color = faith->getColor();
		generatedReligions.emplace(generated.name, generated);
		generatedFaithNames.emplace(faith->getID(), generated.name);
		return generated.name;
	}
	if (const auto& dominant = vanillaPops.getDominantPop(location); dominant && gameDatabase.isValidReligion(dominant->religion))
		return dominant->religion;
	return "catholic";
}

std::string EU5::World::convertCulture(const std::shared_ptr<CK3::Culture>& culture, const std::string& location)
{
	std::optional<std::string> vanillaCulture;
	if (const auto& dominant = vanillaPops.getDominantPop(location); dominant && gameDatabase.isValidCulture(dominant->culture))
		vanillaCulture = dominant->culture;

	if (!culture)
		return vanillaCulture.value_or("swedish");

	// Dynamic (hybrid/divergent) CK3 cultures become real EU5 cultures when their language and
	// heritage map cleanly; the definition is written into the mod.
	if (culture->isDynamic() && dynamicCultures)
	{
		if (const auto& generated = generatedCultureNames.find(culture->getID()); generated != generatedCultureNames.end())
			return generated->second;
		const auto eu5Language = cultureMapper.getEU5LanguageForCK3Language(culture->getLanguage());
		const auto groups = cultureMapper.getEU5GroupsForHeritage(culture->getHeritage());
		if (eu5Language && !groups.empty())
		{
			GeneratedCulture generated;
			generated.name = "conv_cul_" + sanitizeKey(culture->getName());
			generated.rawName = culture->getLocalizedName().value_or(culture->getName());
			// The map may point at a dialect-bearing parent (german_language); the game only
			// accepts leaves, and an invalid language breaks the culture and everyone named by it.
			generated.language = gameDatabase.resolveLanguage(*eu5Language);
			generated.groups = groups;
			// Every vanilla culture has a color, and a player who built a hybrid culture already
			// knows it by the one it wore on the CK3 map, so carry that across.
			generated.color = culture->getColor();
			// Without graphical culture tags the game can't render pops or characters of this
			// culture and logs errors; borrow the tags of a vanilla culture from the same group.
			for (const auto& group: groups)
			{
				for (const auto& relative: gameDatabase.getCulturesInGroup(group))
					if (const auto tags = gameDatabase.getCultureGfxTags(relative); !tags.empty())
					{
						generated.gfxTags = tags;
						break;
					}
				if (!generated.gfxTags.empty())
					break;
			}
			generatedCultures.emplace(generated.name, generated);
			generatedCultureNames.emplace(culture->getID(), generated.name);
			return generated.name;
		}
	}

	// CK3 and EU5 share many culture names outright; a direct hit is the best possible match.
	const auto& cultureName = culture->getName();
	if (gameDatabase.isValidCulture(cultureName))
		return cultureName;
	if (gameDatabase.isValidCulture(cultureName + "_culture"))
		return cultureName + "_culture";

	// The rest of the base cultures have hand-picked links (egyptian -> lower_egyptian_culture...);
	// without one, an Egyptian ruling from Palma would land on whatever Arabic-group culture the
	// group fallback below happens to pick.
	if (const auto mapped = cultureMapper.getEU5CultureForCK3Culture(cultureName))
	{
		if (gameDatabase.isValidCulture(*mapped))
			return *mapped;
		Log(LogLevel::Warning) << "culture_map.txt maps " << cultureName << " to " << *mapped << ", which is not an EU5 culture; ignoring.";
	}

	// No direct match (dynamic/hybrid or renamed culture). Fall back on heritage: if the location's
	// vanilla culture belongs to one of the EU5 groups this heritage maps to, local continuity wins.
	const auto groups = cultureMapper.getEU5GroupsForHeritage(culture->getHeritage());
	if (vanillaCulture && !groups.empty())
	{
		for (const auto& group: groups)
			if (gameDatabase.isCultureInGroup(*vanillaCulture, group))
				return *vanillaCulture;
	}

	// Otherwise pick a culture from the mapped groups, preferring one that speaks the mapped language.
	if (!groups.empty())
	{
		const auto eu5Language = cultureMapper.getEU5LanguageForCK3Language(culture->getLanguage());
		std::vector<std::string> candidates;
		for (const auto& group: groups)
			for (const auto& candidate: gameDatabase.getCulturesInGroup(group))
				candidates.push_back(candidate);
		if (eu5Language)
		{
			for (const auto& candidate: candidates)
				if (gameDatabase.getCultureLanguage(candidate) == *eu5Language)
					return candidate;
		}
		if (!candidates.empty())
			return candidates.front();
	}

	return vanillaCulture.value_or("swedish");
}

std::string EU5::World::determineGovernmentCategory(const std::shared_ptr<CK3::Character>& holder)
{
	std::string ck3Government;
	if (holder->getCharacterDomain())
		ck3Government = holder->getCharacterDomain()->getGovernment();

	if (ck3Government.find("republic") != std::string::npos)
		return "republic";
	if (ck3Government.find("theocracy") != std::string::npos || ck3Government.find("holy_order") != std::string::npos)
		return "theocracy";
	if (ck3Government.find("nomad") != std::string::npos || ck3Government.find("herder") != std::string::npos)
		return "horde";
	if (ck3Government.find("tribal") != std::string::npos)
		return "tribe";
	return "monarchy";
}

std::string EU5::World::determineAIPersonality(const std::shared_ptr<CK3::Character>& holder, const Country& country)
{
	// Vanilla hands out personalities by hand and leaves most countries on the default. A converted
	// world has no such curation, so the ruler's own character decides how their realm behaves:
	// the traits CK3 already uses to drive its AI translate directly.
	auto aggression = 0;
	for (const auto& [index, trait]: holder->getTraits())
	{
		if (trait == "ambitious" || trait == "brave" || trait == "wrathful" || trait == "sadistic" || trait == "adventurer" || trait == "berserker")
			aggression += 2;
		else if (trait == "arrogant" || trait == "impatient" || trait == "vengeful" || trait == "callous" || trait == "eager_reveler")
			++aggression;
		else if (trait == "content" || trait == "humble" || trait == "compassionate" || trait == "forgiving" || trait == "temperate")
			--aggression;
		else if (trait == "craven" || trait == "shy" || trait == "lazy" || trait == "patient")
			aggression -= 2;
	}
	// A tribe of three counties cannot act on grand ambition; an empire hardly needs to.
	const auto large = country.rank == "rank_empire" || country.locations.size() > 60;
	const auto small = country.locations.size() < 8;

	if (aggression >= 3)
		return large ? "ai_aggressive" : "ai_opportunistic";
	if (aggression >= 1)
		return small ? "ai_opportunistic" : "ai_expansionist";
	if (aggression <= -3)
		return small ? "ai_isolationist" : "ai_friendly";
	if (aggression <= -1)
		return small ? "ai_cautious" : "ai_defensive";
	return "ai_balanced";
}

void EU5::World::buildCourt(const std::shared_ptr<CK3::Character>& holder, Country& country, const CK3::World& sourceWorld)
{
	// The knights and councillors around the throne. EU5 fills its cabinet and army commands from
	// the pool of characters a country has, and a converted country that exports only blood
	// relatives has nobody to promote - so the people who actually served the CK3 ruler come along.
	std::set<std::string> exported;
	for (const auto& member: country.family)
		exported.insert(member.key);

	const auto addCourtier = [&](const std::shared_ptr<CK3::Character>& courtier, bool asGeneral) {
		if (!courtier || courtier->isDead())
			return;
		auto converted = convertCharacter(courtier, country, sourceWorld);
		if (exported.contains(converted.key))
			return;
		exported.insert(converted.key);
		converted.tag = country.tag;
		converted.dynastyKey = registerDynasty(courtier, country, sourceWorld);
		// Courtiers are not monarchs; ruler traits belong to whoever sits on the throne.
		converted.rulerTraits.clear();
		converted.estate = "nobles_estate";
		if (asGeneral)
		{
			for (const auto& [index, trait]: courtier->getTraits())
				if (const auto mapping = traitMapper.getGeneralTraitForCK3Trait(trait))
				{
					converted.generalTrait = *mapping;
					break;
				}
			if (converted.generalTrait.empty())
				converted.generalTrait = "goal_oriented"; // a knight with no reputation is still a knight
		}
		country.courtiers.push_back(converted);
	};

	// Keeping every courtier of every realm would double the character database for little gain;
	// the ones who commanded and the ones who governed are enough.
	constexpr size_t maxKnights = 4;
	constexpr size_t maxCouncillors = 5;
	size_t knightCount = 0;
	for (const auto& [knightID, knight]: holder->getKnights())
	{
		if (knightCount >= maxKnights)
			break;
		addCourtier(knight, true);
		++knightCount;
	}
	size_t councillorCount = 0;
	for (const auto& [councillorID, councillor]: holder->getCouncilors())
	{
		if (councillorCount >= maxCouncillors)
			break;
		addCourtier(councillor, false);
		++councillorCount;
	}
}

void EU5::World::buildFamily(const std::shared_ptr<CK3::Character>& holder, Country& country, const CK3::World& sourceWorld)
{
	// Gather the ruler's family by walking parent/spouse/child bonds outwards. Members of the ruler's
	// own dynasty traverse for free, so the entire dynasty tree comes over; every step outside it
	// (spouses, in-laws and their ancestors) costs distance, so foreign courts don't flood the database.
	// The player's realm gets an effectively unlimited budget - that tree is the one being stared at.
	constexpr auto maxDistance = 2;
	const auto isPlayerRealm = sourceWorld.getPlayerTitle() && *sourceWorld.getPlayerTitle() == country.ck3Title;
	const size_t maxMembers = isPlayerRealm ? 10000 : 150;

	long long rulerDynasty = 0;
	if (holder->getHouse().second && holder->getHouse().second->getDynasty().second)
		rulerDynasty = holder->getHouse().second->getDynasty().first;
	const auto sameDynasty = [rulerDynasty](const std::shared_ptr<CK3::Character>& character) {
		return rulerDynasty != 0 && character->getHouse().second && character->getHouse().second->getDynasty().second &&
				 character->getHouse().second->getDynasty().first == rulerDynasty;
	};

	std::map<long long, std::shared_ptr<CK3::Character>> selected;
	std::vector<std::pair<std::shared_ptr<CK3::Character>, int>> queue;
	std::set<long long> queued;
	queue.emplace_back(holder, 0);
	queued.insert(holder->getID());
	for (size_t position = 0; position < queue.size() && selected.size() < maxMembers; ++position)
	{
		// Copy, not reference: enqueueing relatives below can reallocate the queue.
		const auto [member, memberDistance] = queue[position];
		selected.emplace(member->getID(), member);
		const auto enqueue =
			 [&queue, &queued, &sameDynasty](const std::pair<long long, std::shared_ptr<CK3::Character>>& relative, int distance) {
				 if (!relative.second || queued.contains(relative.first))
					 return;
				 queued.insert(relative.first);
				 queue.emplace_back(relative.second, sameDynasty(relative.second) ? 0 : distance);
			 };
		if (memberDistance >= maxDistance)
			continue;
		if (member->getFather())
			enqueue(*member->getFather(), memberDistance + 1);
		if (member->getMother())
			enqueue(*member->getMother(), memberDistance + 1);
		if (member->getSpouse())
			enqueue(*member->getSpouse(), memberDistance + 1);
		for (const auto& child: member->getChildren())
			enqueue(child, memberDistance + 1);
	}

	// Emit in family order - parents strictly before children, so EU5 can wire the tree without crashing.
	std::set<long long> emitted;
	const auto keyFor = [&country](long long ck3ID) {
		return "conv_char_" + std::to_string(ck3ID) + "_" + country.tag;
	};
	std::function<void(const std::shared_ptr<CK3::Character>&)> emit = [&](const std::shared_ptr<CK3::Character>& member) {
		if (emitted.contains(member->getID()))
			return;
		emitted.insert(member->getID());
		if (member->getFather() && selected.contains(member->getFather()->first))
			emit(member->getFather()->second);
		if (member->getMother() && selected.contains(member->getMother()->first))
			emit(member->getMother()->second);

		auto converted = convertCharacter(member, country, sourceWorld);
		converted.tag = country.tag;
		converted.dynastyKey = registerDynasty(member, country, sourceWorld);
		if (member->getFather() && selected.contains(member->getFather()->first))
			converted.fatherKey = keyFor(member->getFather()->first);
		if (member->getMother() && selected.contains(member->getMother()->first))
			converted.motherKey = keyFor(member->getMother()->first);
		if (member->getSpouse() && selected.contains(member->getSpouse()->first))
			converted.spouseKey = keyFor(member->getSpouse()->first);
		country.family.push_back(converted);
	};
	for (const auto& [memberID, member]: selected)
		emit(member);
}

std::string EU5::World::registerDynasty(const std::shared_ptr<CK3::Character>& character, const Country& country, const CK3::World& sourceWorld)
{
	if (!character->getHouse().second)
		return {};
	const auto& house = character->getHouse().second;
	const auto key = "conv_dynasty_" + std::to_string(house->getID());
	if (!dynasties.contains(key))
	{
		ConvertedDynasty dynasty;
		dynasty.key = key;
		if (!house->getLocalizedName().empty())
			dynasty.rawName = house->getLocalizedName();
		else if (const auto& locBlock = sourceWorld.getLocalizationMapper().getLocBlockForKey(house->getName()); locBlock)
			dynasty.rawName = locBlock->english;
		else
			dynasty.rawName = house->getName();
		// Any of the three sources can be a loc reference ("$dynn_Nakamikado$") rather than a name.
		dynasty.rawName = resolveNestedName(dynasty.rawName, sourceWorld.getLocalizationMapper());
		dynasty.home = country.capital;
		// EU5 looks up dynasty coats of arms by dynasty key; without one the game shows a
		// generic fallback, so carry the CK3 dynasty arms over.
		if (house->getDynasty().second && house->getDynasty().second->getCoA() && house->getDynasty().second->getCoA()->second)
			dynasty.coa = house->getDynasty().second->getCoA()->second;
		dynasties.emplace(key, dynasty);
	}
	return key;
}

EU5::ConvertedCharacter EU5::World::convertCharacter(const std::shared_ptr<CK3::Character>& character,
	 const Country& country,
	 const CK3::World& sourceWorld)
{
	ConvertedCharacter converted;
	// Keys are per-country: the same CK3 character can appear for multiple countries (ruler of two realms,
	// heir in one and consort in another), and each country needs a character carrying its own tag.
	converted.key = "conv_char_" + std::to_string(character->getID()) + "_" + country.tag;
	// Character names in the save are CK3 loc keys (Adelai_da, Yamato_3...); resolve them to display text.
	converted.rawName = character->getName();
	if (const auto& locBlock = sourceWorld.getLocalizationMapper().getLocBlockForKey(character->getName()); locBlock && !locBlock->english.empty())
		converted.rawName = locBlock->english;
	// Localized values may themselves be references ("$name_key$"); resolve until only text remains.
	converted.rawName = resolveNestedName(converted.rawName, sourceWorld.getLocalizationMapper());
	converted.nameKey = "conv_name_" + sanitizeKey(character->getName());
	converted.female = character->isFemale();
	converted.birthDate = character->getBirthDate();
	converted.birthDate.ChangeByYears(yearOffset);
	if (character->isDead())
	{
		// Dead relatives become dynasty history. The odd corpse without a recorded date gets one invented.
		auto death = character->getDeathDate().value_or(date("1336.1.1"));
		if (character->getDeathDate())
			death.ChangeByYears(yearOffset);
		converted.deathDate = death;
	}
	converted.birthLocation = country.capital;
	converted.culture = country.culture;
	converted.religion = country.religion;
	// Characters keep their own faith and culture where the mapping is solid; the same fallback
	// paths as the realm's keep rulers from diverging from their own realm (catholic-in-paulician bugs).
	if (character->getCulture() && character->getCulture()->second)
	{
		const auto characterCulture = convertCulture(character->getCulture()->second, country.capital);
		if (isValidOrGeneratedCulture(characterCulture))
			converted.culture = characterCulture;
	}
	if (character->getFaith() && character->getFaith()->second)
	{
		const auto characterFaith = convertFaith(character->getFaith()->second, country.capital);
		if (isValidOrGeneratedReligion(characterFaith))
			converted.religion = characterFaith;
	}
	converted.adm = scaleSkill(character->getSkills().stewardship);
	converted.dip = scaleSkill(character->getSkills().diplomacy);
	converted.mil = scaleSkill(character->getSkills().martial);
	// Everyone the converter exports came out of a CK3 noble court, which is the nobility as far as
	// EU5 is concerned. Rulers of theocracies and republics are moved to their own estate later.
	converted.estate = "nobles_estate";

	// CK3 personality traits carry over as EU5 ruler traits; the engine tolerates two per character.
	constexpr size_t maxRulerTraits = 2;
	std::set<std::string> assigned;
	for (const auto& [index, trait]: character->getTraits())
	{
		if (converted.rulerTraits.size() >= maxRulerTraits)
			break;
		const auto mapping = traitMapper.getEU5TraitForCK3Trait(trait);
		if (!mapping || assigned.contains(*mapping))
			continue;
		converted.rulerTraits.push_back(*mapping);
		assigned.insert(*mapping);
	}
	return converted;
}
