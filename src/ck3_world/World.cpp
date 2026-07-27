#include "World.h"
#include "Characters/Character.h"
#include "Characters/CharacterDomain.h"
#include "CommonFunctions.h"
#include "CommonRegexes.h"
#include "Geography/CountyDetail.h"
#include "Log.h"
#include "ModLoader/ModFilesystem.h"
#include "OSCompatibilityLayer.h"
#include "ParserHelpers.h"
#include "Religions/Faith.h"
#include "Religions/Religion.h"
#include "Titles/Title.h"
#include <external/rakaly/rakaly.h>
#include <filesystem>
#include <fstream>
#include <ranges>
namespace fs = std::filesystem;

namespace
{
// CK3 keeps its actual game data under <install>/game/. Older layouts (and test fixtures) may point directly at the data root.
std::filesystem::path GetCK3GameDirectory(const configuration::Configuration& theConfiguration)
{
	const auto& ck3Path = theConfiguration.GetCK3Directory();
	if (commonItems::DoesFolderExist(ck3Path / "game"))
		return ck3Path / "game";
	return ck3Path;
}
} // namespace

CK3::World::World(const configuration::Configuration& theConfiguration, const commonItems::ConverterVersion& converterVersion)
{
	registerKeys(theConfiguration, converterVersion);
	Log(LogLevel::Progress) << "4 %";

	Log(LogLevel::Info) << "-> Verifying CK3 save.";
	verifySave(theConfiguration.GetSaveGamePath());
	processSave(theConfiguration.GetSaveGamePath(), theConfiguration.GetDebug());
	Log(LogLevel::Progress) << "5 %";

	auto metaData = std::istringstream(saveGame.metadata);
	parseStream(metaData);
	Log(LogLevel::Progress) << "10 %";

	Log(LogLevel::Info) << "* Priming Converter Components *";
	primeLaFabricaDeColor(theConfiguration);
	loadLandedTitles(theConfiguration);
	loadCharacterTraits(theConfiguration);
	loadHouseNames(theConfiguration);
	Log(LogLevel::Progress) << "15 %";
	// Scraping localizations from CK3 so we may know proper names for our countries and people.
	Log(LogLevel::Info) << "-> Reading Words";
	localizationMapper.scrapeLocalizations(GetCK3GameDirectory(theConfiguration), mods);

	Log(LogLevel::Info) << "* Parsing Gamestate *";
	auto gameState = std::istringstream(saveGame.gamestate);
	parseStream(gameState);
	Log(LogLevel::Progress) << "20 %";
	clearRegisteredKeywords();

	Log(LogLevel::Info) << "* Gamestate Parsing Complete, Weaving Internals *";
	crosslinkDatabases();
	Log(LogLevel::Progress) << "30 %";

	// processing
	Log(LogLevel::Info) << "-- Checking For Religions";
	checkForIslam();
	Log(LogLevel::Info) << "-- Filtering Independent Titles";
	filterIndependentTitles();
	Log(LogLevel::Info) << "-- Rounding Up Some People";
	gatherCourtierNames();
	Log(LogLevel::Info) << "-- Congregating DeFacto Counties for Independent Titles";
	congregateDFCounties();
	Log(LogLevel::Info) << "-- Congregating DeJure Counties for Independent Titles";
	congregateDJCounties();
	Log(LogLevel::Info) << "-- Filtering Landless Titles";
	filterLandlessTitles();

	if (playerID)
	{
		Log(LogLevel::Info) << "-- Locating Player Title.";
		locatePlayerTitle();
	}

	Log(LogLevel::Info) << "*** Good-bye CK3, rest in peace. ***";
	Log(LogLevel::Progress) << "47 %";
}

void CK3::World::registerKeys(const configuration::Configuration& theConfiguration, const commonItems::ConverterVersion& converterVersion)
{
	Log(LogLevel::Info) << "*** Hello CK3, Deus Vult! ***";

	metaParser.registerKeyword("mods", [this, theConfiguration](std::istream& theStream) {
		Log(LogLevel::Info) << "-> Detecting used mods.";
		std::set<std::string> seenMods;
		for (const auto& path: commonItems::getStrings(theStream))
		{
			if (seenMods.contains(path))
				continue;
			mods.emplace_back(Mod("", path));
			seenMods.emplace(path);
		}
		Log(LogLevel::Info) << "<> Savegame claims " << mods.size() << " mods used.";
		commonItems::ModLoader modLoader;
		modLoader.loadMods(theConfiguration.GetCK3DocDirectory(), mods);
		mods = modLoader.getMods();
	});
	metaParser.registerKeyword("meta_title_name", [this](std::istream& theStream) {
		// The realm name as CK3 displays it (e.g. "the Yamamoto Empire") - dynamic nomad/adventurer
		// titles often carry a stale internal name, so this is the better source for the player realm.
		metaTitleName = commonItems::getString(theStream);
	});
	metaParser.registerKeyword("meta_coat_of_arms", [this](std::istream& theStream) {
		// The realm arms as CK3 displays them - for dynamic realms these are the house arms, not the title's.
		metaCoA = std::make_shared<CoatOfArms>(theStream, 0);
	});
	metaParser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

	registerRegex("SAV.*", [](const std::string&, std::istream&) {
	});
	registerKeyword("meta_data", [this](std::istream& theStream) {
		if (saveGame.parsedMeta)
		{
			commonItems::ignoreItem("unused", theStream);
		}
		else
		{
			metaParser.parseStream(theStream);
			saveGame.parsedMeta = true;
		}
	});
	registerKeyword("currently_played_characters", [this](std::istream& theStream) {
		auto playedCharacters = commonItems::getLlongs(theStream);
		if (!playedCharacters.empty())
			playerID = playedCharacters[0];
	});
	registerKeyword("date", [this](const std::string&, std::istream& theStream) {
		const commonItems::singleString dateString(theStream);
		endDate = date(dateString.getString());
	});
	registerKeyword("bookmark_date", [this](const std::string&, std::istream& theStream) {
		const commonItems::singleString startDateString(theStream);
		startDate = date(startDateString.getString());
	});
	registerKeyword("version", [this, converterVersion](const std::string&, std::istream& theStream) {
		const commonItems::singleString versionString(theStream);
		CK3Version = GameVersion(versionString.getString());
		Log(LogLevel::Info) << "<> Savegame version: " << versionString.getString();

		if (converterVersion.getMinSource() > CK3Version)
		{
			Log(LogLevel::Error) << "Converter requires a minimum save from v" << converterVersion.getMinSource().toShortString();
			throw std::runtime_error("Savegame vs converter version mismatch!");
		}
		if (!converterVersion.getMaxSource().isLargerishThan(CK3Version))
		{
			Log(LogLevel::Error) << "Converter requires a maximum save from v" << converterVersion.getMaxSource().toShortString();
			throw std::runtime_error("Savegame vs converter version mismatch!");
		}
	});
	registerKeyword("variables", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading variable flags.";
		flags = Flags(theStream);
		Log(LogLevel::Info) << "<> Loaded " << flags.getFlags().size() << " variable flags.";
	});
	registerKeyword("landed_titles", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading titles.";
		titles = Titles(theStream);
		const auto& counter = titles.getCounter();
		Log(LogLevel::Info) << "<> Loaded " << titles.getTitles().size() << " titles: " << counter[0] << "b " << counter[1] << "c " << counter[2] << "d "
								  << counter[3] << "k " << counter[4] << "e " << counter[5] << "h" << counter[6] << " dynamics.";
	});
	registerKeyword("provinces", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading provinces.";
		provinceHoldings = ProvinceHoldings(theStream);
		Log(LogLevel::Info) << "<> Loaded " << provinceHoldings.getProvinceHoldings().size() << " provinces.";
	});
	registerKeyword("living", [this](std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading potentially alive human beings.";
		characters.loadCharacters(theStream);
		Log(LogLevel::Info) << "<> Loaded " << characters.getCharacters().size() << " human entities.";
	});
	registerKeyword("dead_unprunable", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading dead people.";
		characters.loadCharacters(theStream);
		Log(LogLevel::Info) << "<> Loaded " << characters.getCharacters().size() << " human remains.";
	});
	registerKeyword("dynasties", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading dynasties.";
		dynasties = Dynasties(theStream);
		houses = dynasties.getHouses(); // Do not access houses in dynasties after this - there are none and will crash.
		Log(LogLevel::Info) << "<> Loaded " << dynasties.getDynasties().size() << " dynasties and " << houses.getHouses().size() << " houses.";
	});
	registerKeyword("religion", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading religions.";
		religions = Religions(theStream);
		faiths = religions.getFaiths(); // Do not access faiths in religions after this - there are none and will crash.
		Log(LogLevel::Info) << "<> Loaded " << religions.getReligions().size() << " religions and " << faiths.getFaiths().size() << " faiths.";
	});
	registerKeyword("coat_of_arms", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading garments of limbs.";
		coats = CoatsOfArms(theStream);
		Log(LogLevel::Info) << "<> Loaded " << coats.getCoats().size() << " wearables.";
	});
	registerKeyword("county_manager", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading county details.";
		countyDetails = CountyDetails(theStream);
		Log(LogLevel::Info) << "<> Loaded " << countyDetails.getCountyDetails().size() << " county details.";
	});
	registerKeyword("culture_manager", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading cultures.";
		cultures = Cultures(theStream);
		Log(LogLevel::Info) << "<> Loaded " << cultures.getCultures().size() << " cultures.";
	});
	registerKeyword("confederation_manager", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading confederations.";
		confederations = Confederations(theStream);
		Log(LogLevel::Info) << "<> Loaded " << confederations.getConfederations().size() << " confederations.";
	});
	registerKeyword("relations", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading relations.";
		relations = Relations(theStream);
		Log(LogLevel::Info) << "<> Loaded " << relations.getAlliancePairs().size() << " alliances.";
	});
	registerKeyword("opinions", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading opinions.";
		opinions = Opinions(theStream);
		Log(LogLevel::Info) << "<> Loaded " << opinions.getRivalPairs().size() << " rivalries.";
	});
	registerKeyword("wars", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading wars.";
		wars = Wars(theStream);
		Log(LogLevel::Info) << "<> Loaded " << wars.getWars().size() << " active wars.";
	});
	registerKeyword("armies", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading men-at-arms.";
		armies = Armies(theStream);
		Log(LogLevel::Info) << "<> Loaded " << armies.getRegimentCount() << " men-at-arms regiments held by " << armies.getMenAtArms().size() << " rulers.";
	});
	registerKeyword("vassal_contracts", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading vassal contracts.";
		vassalContracts = VassalContracts(theStream);
		Log(LogLevel::Info) << "<> Loaded " << vassalContracts.getContractGroups().size() << " vassal contracts.";
	});
	registerKeyword("artifacts", [this](const std::string&, std::istream& theStream) {
		Log(LogLevel::Info) << "-> Loading artifacts.";
		artifacts = Artifacts(theStream);
		Log(LogLevel::Info) << "<> Loaded " << artifacts.getArtifacts().size() << " artifacts.";
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

void CK3::World::locatePlayerTitle()
{
	for (const auto& title: independentTitles)
		if (title.second->getHolder() && title.second->getHolder()->first == *playerID)
		{
			Log(LogLevel::Info) << "Player title: " << title.second->getName();
			playerTitle = title.first;
			break;
		}
}

void CK3::World::processSave(const std::filesystem::path& saveGamePath, bool debug)
{
	std::ifstream saveFile(saveGamePath, std::ios::binary);
	std::stringstream inStream;
	inStream << saveFile.rdbuf();
	saveGame.gamestate = inStream.str();

	const auto save = rakaly::parseCk3(saveGame.gamestate);
	if (const auto& melt = save.meltMeta(); melt)
	{
		Log(LogLevel::Info) << "Meta extracted successfully.";
		melt->writeData(saveGame.metadata);
	}
	else if (save.is_binary())
	{
		Log(LogLevel::Error) << "Binary Save and NO META!";
	}

	if (save.is_binary())
	{
		Log(LogLevel::Info) << "Gamestate is binary, melting.";
		const auto& melt = save.melt();
		if (melt.has_unknown_tokens())
		{
			Log(LogLevel::Error) << "Rakaly reports errors while melting ironman save!";
		}

		melt.writeData(saveGame.gamestate);
	}
	else
	{
		Log(LogLevel::Info) << "Gamestate is textual.";
		const auto& melt = save.melt();
		melt.writeData(saveGame.gamestate);
	}

	if (debug)
	{
		std::ofstream metaDump("metaDump.txt");
		metaDump << saveGame.metadata;
		metaDump.close();

		std::ofstream saveDump("saveDump.txt");
		saveDump << saveGame.gamestate;
		saveDump.close();
	}
}

void CK3::World::verifySave(const std::filesystem::path& saveGamePath) const
{
	std::ifstream saveFile(saveGamePath, std::ios::binary);
	if (!saveFile.is_open())
		throw std::runtime_error("Could not open save! Exiting!");

	char buffer[10];
	saveFile.get(buffer, 4);
	if (buffer[0] != 'S' || buffer[1] != 'A' || buffer[2] != 'V')
		throw std::runtime_error("Savefile of unknown type.");

	saveFile.close();
}

void CK3::World::primeLaFabricaDeColor(const configuration::Configuration& theConfiguration)
{
	Log(LogLevel::Info) << "-> Loading colors.";
	for (const auto& file: commonItems::GetAllFilesInFolder(GetCK3GameDirectory(theConfiguration) / "common/named_colors"))
	{
		if (file.extension() != ".txt")
			continue;
		namedColors.loadColors(GetCK3GameDirectory(theConfiguration) / "common/named_colors" / file);
	}
	for (const auto& mod: mods)
	{
		if (!commonItems::DoesFolderExist(mod.path / "common/named_colors"))
			continue;
		Log(LogLevel::Info) << "<> Loading some colors from [" << mod.name << "]";
		for (const auto& file: commonItems::GetAllFilesInFolder(mod.path / "common/named_colors"))
		{
			if (file.extension() != ".txt")
				continue;
			namedColors.loadColors(mod.path / "common/named_colors" / file);
		}
	}
	Log(LogLevel::Info) << "<> Loaded " << laFabricaDeColor.getRegisteredColors().size() << " colors.";
}

void CK3::World::loadLandedTitles(const configuration::Configuration& theConfiguration)
{
	Log(LogLevel::Info) << "-> Loading Landed Titles.";
	commonItems::ModFilesystem modFS(GetCK3GameDirectory(theConfiguration), mods);
	for (const auto& file: modFS.GetAllFilesInFolder("common/landed_titles"))
	{
		if (file.extension() != ".txt")
			continue;
		landedTitles.loadTitles(file);
	}
	Log(LogLevel::Info) << "<> Loaded " << landedTitles.getFoundTitles().size() << " landed titles.";
}

void CK3::World::loadCharacterTraits(const configuration::Configuration& theConfiguration)
{
	Log(LogLevel::Info) << "-> Examiming Personalities";
	for (const auto& file: commonItems::GetAllFilesInFolder(GetCK3GameDirectory(theConfiguration) / "common/traits"))
	{
		if (file.extension() != ".txt")
			continue;
		traitScraper.loadTraits(GetCK3GameDirectory(theConfiguration) / "common/traits" / file);
	}
	for (const auto& mod: mods)
	{
		if (!commonItems::DoesFolderExist(mod.path / "common/traits"))
			continue;
		Log(LogLevel::Info) << "<> Loading some character traits from [" << mod.name << "]";
		for (const auto& file: commonItems::GetAllFilesInFolder(mod.path / "common/traits"))
		{
			if (file.extension() != ".txt")
				continue;
			traitScraper.loadTraits(mod.path / "common/traits" / file);
		}
	}
	Log(LogLevel::Info) << ">> " << traitScraper.getTraits().size() << " personalities scrutinized.";
}

void CK3::World::loadHouseNames(const configuration::Configuration& theConfiguration)
{
	Log(LogLevel::Info) << "-> Loading House Names";
	for (const auto& file: commonItems::GetAllFilesInFolder(GetCK3GameDirectory(theConfiguration) / "common/dynasty_houses"))
	{
		if (file.extension() != ".txt")
			continue;
		houseNameScraper.loadHouseDetails(GetCK3GameDirectory(theConfiguration) / "common/dynasty_houses" / file);
	}
	for (const auto& mod: mods)
	{
		if (!commonItems::DoesFolderExist(mod.path / "common/dynasty_houses"))
			continue;
		Log(LogLevel::Info) << "<> Loading house names from [" << mod.name << "]";
		for (const auto& file: commonItems::GetAllFilesInFolder(mod.path / "common/dynasty_houses"))
		{
			if (file.extension() != ".txt")
				continue;
			houseNameScraper.loadHouseDetails(mod.path / "common/dynasty_houses" / file);
		}
	}
	Log(LogLevel::Info) << ">> " << houseNameScraper.getHouseNames().size() << " house names read.";
}

void CK3::World::crosslinkDatabases()
{
	Log(LogLevel::Info) << "-> Injecting Names into Houses.";
	houses.importNames(houseNameScraper);
	Log(LogLevel::Info) << "-> Loading Cultures into Counties.";
	countyDetails.linkCultures(cultures);
	Log(LogLevel::Info) << "-> Loading Cultures into Characters.";
	characters.linkCultures(cultures);
	Log(LogLevel::Info) << "-> Loading Faiths into Counties.";
	countyDetails.linkFaiths(faiths);
	Log(LogLevel::Info) << "-> Loading Faiths into Characters.";
	characters.linkFaiths(faiths);
	Log(LogLevel::Info) << "-> Loading Faiths into Religions.";
	religions.linkFaiths(faiths);
	Log(LogLevel::Info) << "-> Loading Religions into Faiths.";
	faiths.linkReligions(religions, titles);
	Log(LogLevel::Info) << "-> Loading Titles into Coats.";
	coats.linkParents(titles);
	Log(LogLevel::Info) << "-> Loading Coats into Dynasties.";
	dynasties.linkCoats(coats);
	Log(LogLevel::Info) << "-> Loading Coats into Titles.";
	titles.linkCoats(coats);
	Log(LogLevel::Info) << "-> Loading Holdings into Clay.";
	landedTitles.linkProvinceHoldings(provinceHoldings);
	Log(LogLevel::Info) << "-> Loading Counties into Clay.";
	landedTitles.linkCountyDetails(countyDetails);
	Log(LogLevel::Info) << "-> Loading Dynasties into Houses.";
	houses.linkDynasties(dynasties);
	Log(LogLevel::Info) << "-> Loading Characters into Houses.";
	houses.linkCharacters(characters);
	Log(LogLevel::Info) << "-> Loading Houses into Characters.";
	characters.linkHouses(houses);
	Log(LogLevel::Info) << "-> Loading Characters into Titles.";
	titles.linkCharacters(characters);
	Log(LogLevel::Info) << "-> Loading Titles into Characters.";
	characters.linkTitles(titles);
	Log(LogLevel::Info) << "-> Loading Titles into Titles.";
	titles.linkTitles();
	Log(LogLevel::Info) << "-> Fixing Titles Pointing To Wrong Places.";
	titles.relinkDeFactoVassals();
	Log(LogLevel::Info) << "-> Loading Titles into Clay.";
	landedTitles.linkTitles(titles);
	Log(LogLevel::Info) << "-> Loading Characters into Characters.";
	characters.linkCharacters();
	Log(LogLevel::Info) << "-> Linking Families.";
	characters.linkFamilies();
	Log(LogLevel::Info) << "-> Loading Clay into Titles.";
	titles.linkLandedTitles(landedTitles);
	Log(LogLevel::Info) << "-> Loading Traits into Characters.";
	characters.linkTraits(traitScraper);
	Log(LogLevel::Info) << "-> Loading Coats into Confederations.";
	confederations.linkCoats(coats);
}

void CK3::World::checkForIslam()
{
	for (const auto& county: countyDetails.getCountyDetails() | std::views::values)
	{
		if (!county->getFaith().second)
			continue;
		if (!county->getFaith().second->getReligion().second)
			continue;
		if (county->getFaith().second->getReligion().second->getName() == "islam_religion")
		{
			islamExists = true;
			return;
		}
	}
}

void CK3::World::filterIndependentTitles()
{
	const auto& allTitles = titles.getTitles();
	std::map<std::string, std::shared_ptr<Title>> potentialIndeps;

	for (const auto& title: allTitles)
	{
		if (!title.second->getHolder())
			continue; // don't bother with titles without holders.
		if (!title.second->getDFLiege())
		{
			// this is a potential indep.
			potentialIndeps.insert(title);
		}
		if (title.second->getDFLiege() && !title.second->getDFLiege()->second->getHolder()) // yes, we can have a dfliege that's destroyed, apparently.
		{
			// this is also potential indep.
			potentialIndeps.insert(title);
			// And do fix it.
			title.second->grantIndependence();
		}
	}

	// Check if the holder holds any actual land (b|c_something). (Only necessary for the holder,
	// no need to recurse, we're just filtering landless titular titles like mercenaries
	// or landless Pope. If a character holds a landless titular title along actual title
	// (like Caliphate), it's not relevant at this stage as he's independent anyway.

	// First, split off all county_title holders into a container.
	std::set<long long> countyHolders;
	std::map<long long, std::map<std::string, std::shared_ptr<Title>>> allTitleHolders;
	for (const auto& title: allTitles)
	{
		if (title.second->getHolder())
		{
			if (title.second->getLevel() == LEVEL::COUNTY)
				countyHolders.insert(title.second->getHolder()->first);
			allTitleHolders[title.second->getHolder()->first].insert(title);
		}
	}

	// Then look at all potential indeps and see if their holders hold physical clay.
	auto counter = 0;
	for (const auto& indep: potentialIndeps)
	{
		const auto& holderID = indep.second->getHolder()->first;
		if (countyHolders.count(holderID))
		{
			// this fellow holds a county, so his indep title is an actual title.
			independentTitles.insert(indep);
			counter++;
			// Set The Pope
			if (indep.first == "k_papal_state")
			{
				indep.second->setThePope();
				Log(LogLevel::Info) << "---> " << indep.first << " is the Pope.";
			}
			else
			{
				if (allTitleHolders[holderID].count("k_papal_state"))
				{
					indep.second->setThePope();
					Log(LogLevel::Info) << "---> " << indep.first << " belongs to the Pope.";
				}
			}
		}
	}
	Log(LogLevel::Info) << "<> " << counter << " independent titles recognized.";
}

void CK3::World::gatherCourtierNames()
{
	// We're using this function to locate courtiers, assemble their names as potential Monarch Names in EU5,
	// and also while at it, to see if they hold adviser jobs. It's anything but trivial, as being employed doesn't equate with
	// being a councilor, nor do landed councilors have employers if they work for their liege.

	auto counter = 0;
	auto counterAdvisors = 0;
	auto counterKnights = 0;
	std::map<long long, std::map<std::string, bool>> holderCourtiers;								// holder-name/male
	std::map<long long, std::map<long long, std::shared_ptr<Character>>> holderCouncilors; // holder-councilors
	std::map<long long, std::map<long long, std::shared_ptr<Character>>> holderKnights;	  // holder-knights

	for (const auto& character: characters.getCharacters())
	{
		// Do you even exist?
		if (!character.second)
			continue;
		// Hello. Are you an employed individual?
		if (!character.second->isCouncilor() && !character.second->getEmployer())
			continue;
		// If you have a steady job, we need your employer's references.
		if (character.second->isCouncilor())
		{
			if (character.second->getEmployer() && character.second->getEmployer()->second)
			{
				// easiest case.
				holderCourtiers[character.second->getEmployer()->first].insert(std::pair(character.second->getName(), !character.second->isFemale()));
				holderCouncilors[character.second->getEmployer()->first].insert(character);
			}
			else if (character.second->getCharacterDomain() && !character.second->getCharacterDomain()->getDomain().empty())
			{
				// this councilor is landed and works for his liege.
				const auto& characterPrimaryTitle = character.second->getCharacterDomain()->getDomain()[0];
				if (!characterPrimaryTitle.second)
					continue; // corruption
				const auto& liegeTitle = characterPrimaryTitle.second->getDFLiege();
				if (!liegeTitle || !liegeTitle->second)
					continue; // I dislike this character. I think it is time he was let go.
				const auto& liege = liegeTitle->second->getHolder();
				if (!liege || !liege->second)
					continue; // Or maybe we should fire his liege.
				holderCourtiers[liege->first].insert(std::pair(character.second->getName(), character.second->isFemale()));
				holderCouncilors[liege->first].insert(character);
			}
			else
			{
				// Doesn't have employer and doesn't have land but is councilor. Bollocks.
				continue;
			}
		}
		else if (character.second->getEmployer())
		{
			// Being employed but without a council task means a knight or physician or similar. Works for us.
			holderCourtiers[character.second->getEmployer()->first].insert(std::pair(character.second->getName(), !character.second->isFemale()));
			// Knights are the realm's fighting men; EU5 wants them as generals.
			if (character.second->isKnight() && !character.second->isDead())
				holderKnights[character.second->getEmployer()->first].insert(character);
		}
	}

	// We're only interested in those working for indeps.
	for (const auto& title: independentTitles)
	{
		if (!title.second->getHolder() || !title.second->getHolder()->second)
			continue; // Nobody home to hire anyone.
		const auto containerItr = holderCourtiers.find(title.second->getHolder()->first);
		if (containerItr != holderCourtiers.end())
		{
			title.second->getHolder()->second->loadCourtierNames(containerItr->second);
			counter += static_cast<int>(containerItr->second.size());
		}
		const auto councilorItr = holderCouncilors.find(title.second->getHolder()->first);
		if (councilorItr != holderCouncilors.end())
		{
			title.second->getHolder()->second->loadCouncilors(councilorItr->second);
			counterAdvisors += static_cast<int>(councilorItr->second.size());
		}
		const auto knightItr = holderKnights.find(title.second->getHolder()->first);
		if (knightItr != holderKnights.end())
		{
			title.second->getHolder()->second->loadKnights(knightItr->second);
			counterKnights += static_cast<int>(knightItr->second.size());
		}
	}
	Log(LogLevel::Info) << "<> " << counter << " people gathered for interrogation. " << counterAdvisors << " were detained, " << counterKnights
							  << " were armed.";
}

void CK3::World::congregateDFCounties()
{
	auto counter = 0;
	// We're linking all contained counties for a title's tree under that title.
	// This will form actual EU5 country and contained locations.
	for (const auto& title: independentTitles)
	{
		title.second->congregateDFCounties();
		for (const auto& province: title.second->getOwnedDFCounties())
		{
			province.second->loadHoldingTitle(std::pair(title.first, title.second));
		}
		counter += static_cast<int>(title.second->getOwnedDFCounties().size());
	}
	Log(LogLevel::Info) << "<> " << counter << " counties held by independents.";
}

void CK3::World::congregateDJCounties()
{
	auto counter = 0;
	// We're linking all dejure provinces under the title as these will be the base
	// for that title's permanent claims, unless already owned.
	for (const auto& title: independentTitles)
	{
		title.second->congregateDJCounties();
		counter += static_cast<int>(title.second->getOwnedDJCounties().size());
	}
	Log(LogLevel::Info) << "<> " << counter << " de jure provinces claimed by independents.";
}

void CK3::World::filterLandlessTitles()
{
	auto counter = 0;
	std::set<std::string> titlesForDisposal;
	for (const auto& title: independentTitles)
	{
		if (title.second->getOwnedDFCounties().empty())
		{
			titlesForDisposal.insert(title.first);
		}
	}
	for (const auto& drop: titlesForDisposal)
	{
		independentTitles.erase(drop);
		counter++;
	}
	Log(LogLevel::Info) << "<> " << counter << " empty titles dropped, " << independentTitles.size() << " remain.";
}
