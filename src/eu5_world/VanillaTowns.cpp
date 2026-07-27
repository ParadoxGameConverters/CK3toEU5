#include "VanillaTowns.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"

namespace
{
class TownEntry: commonItems::parser
{
  public:
	explicit TownEntry(std::istream& theStream)
	{
		registerKeyword("rank", [this](std::istream& stream) {
			rank = commonItems::getString(stream);
		});
		registerKeyword("town_setup", [this](std::istream& stream) {
			setup = commonItems::getString(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string rank;
	std::string setup;
};
} // namespace

void EU5::VanillaTowns::loadTowns(const std::filesystem::path& citiesFile)
{
	registerKeys();
	parseFile(citiesFile);
	clearRegisteredKeywords();
}

void EU5::VanillaTowns::loadTowns(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void EU5::VanillaTowns::registerKeys()
{
	registerKeyword("locations", [this](std::istream& theStream) {
		commonItems::parser locationsParser;
		locationsParser.registerRegex(commonItems::catchallRegex, [this](const std::string& location, std::istream& stream) {
			const TownEntry entry(stream);
			// Plenty of locations appear here carrying other setup data; only the ranked ones are towns.
			if (!entry.rank.empty())
				towns[location] = VanillaTown{entry.rank, entry.setup};
		});
		locationsParser.parseStream(theStream);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

bool EU5::VanillaTowns::isCity(const std::string& location) const
{
	const auto& match = towns.find(location);
	// Megalopolis outranks city; both are past the point where a converted town could be promoted.
	return match != towns.end() && (match->second.rank == "city" || match->second.rank == "megalopolis");
}
