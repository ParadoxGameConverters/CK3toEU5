#include "UnitMapper.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"

namespace
{
class UnitLink: commonItems::parser
{
  public:
	explicit UnitLink(std::istream& theStream)
	{
		registerKeyword("category", [this](std::istream& stream) {
			category = commonItems::getString(stream);
		});
		registerKeyword("unit", [this](std::istream& stream) {
			units.emplace_back(commonItems::getString(stream), 1);
		});
		registerKeyword("ratio", [this](std::istream& stream) {
			if (!units.empty())
				units.back().second = commonItems::getInt(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string category;
	std::vector<std::pair<std::string, int>> units;
};
} // namespace

mappers::UnitMapper::UnitMapper(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::UnitMapper::loadMappings(const std::filesystem::path& fileName)
{
	registerKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::UnitMapper::registerKeys()
{
	registerKeyword("link", [this](std::istream& theStream) {
		const UnitLink link(theStream);
		if (link.category.empty() || link.units.empty())
			return;
		compositions[link.category] = link.units;
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

std::vector<std::string> mappers::UnitMapper::getRegiments(const std::string& category, int count) const
{
	std::vector<std::string> regiments;
	if (count <= 0)
		return regiments;
	auto composition = compositions.find(category);
	if (composition == compositions.end())
		composition = compositions.find("feudal");
	if (composition == compositions.end())
	{
		regiments.assign(static_cast<size_t>(count), "a_footmen");
		return regiments;
	}
	auto totalWeight = 0;
	for (const auto& [unit, ratio]: composition->second)
		totalWeight += ratio;
	if (totalWeight <= 0)
		totalWeight = 1;
	// Deal regiments out proportionally; the first (main) unit absorbs the rounding remainder.
	auto assigned = 0;
	for (const auto& [unit, ratio]: composition->second)
	{
		const auto share = count * ratio / totalWeight;
		for (auto regiment = 0; regiment < share; ++regiment)
			regiments.push_back(unit);
		assigned += share;
	}
	const auto& mainUnit = composition->second.front().first;
	for (; assigned < count; ++assigned)
		regiments.insert(regiments.begin(), mainUnit);
	return regiments;
}
