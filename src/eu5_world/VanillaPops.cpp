#include "VanillaPops.h"
#include "CommonRegexes.h"
#include "Log.h"
#include "ParserHelpers.h"

namespace
{
class PopEntry: commonItems::parser
{
  public:
	explicit PopEntry(std::istream& theStream)
	{
		registerKeyword("type", [this](std::istream& stream) {
			type = commonItems::getString(stream);
		});
		registerKeyword("size", [this](std::istream& stream) {
			size = commonItems::getDouble(stream);
		});
		registerKeyword("culture", [this](std::istream& stream) {
			culture = commonItems::getString(stream);
		});
		registerKeyword("religion", [this](std::istream& stream) {
			religion = commonItems::getString(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string type;
	double size = 0;
	std::string culture;
	std::string religion;
};

class LocationPops: commonItems::parser
{
  public:
	explicit LocationPops(std::istream& theStream)
	{
		registerKeyword("define_pop", [this](std::istream& stream) {
			const PopEntry pop(stream);
			if (pop.culture.empty())
				return;
			pops.push_back(EU5::Pop{pop.type, pop.size, pop.culture, pop.religion});
			if (pop.size <= biggestSize)
				return;
			biggestSize = pop.size;
			culture = pop.culture;
			religion = pop.religion;
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	double biggestSize = 0;
	std::string culture;
	std::string religion;
	std::vector<EU5::Pop> pops;
};
} // namespace

void EU5::VanillaPops::loadPops(const std::filesystem::path& popsFile)
{
	registerKeys();
	parseFile(popsFile);
	clearRegisteredKeywords();
}

void EU5::VanillaPops::loadPops(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void EU5::VanillaPops::registerKeys()
{
	registerKeyword("locations", [this](std::istream& theStream) {
		commonItems::parser locationsParser;
		locationsParser.registerRegex(commonItems::catchallRegex, [this](const std::string& location, std::istream& stream) {
			const LocationPops pops(stream);
			if (!pops.culture.empty())
				dominantPops[location] = DominantPop{pops.culture, pops.religion};
			if (!pops.pops.empty())
				locationPops[location] = pops.pops;
		});
		locationsParser.parseStream(theStream);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

std::optional<EU5::DominantPop> EU5::VanillaPops::getDominantPop(const std::string& location) const
{
	if (const auto& match = dominantPops.find(location); match != dominantPops.end())
		return match->second;
	return std::nullopt;
}
