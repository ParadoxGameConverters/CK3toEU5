#include "BuildingMapper.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"

namespace
{
class BuildingLink: commonItems::parser
{
  public:
	explicit BuildingLink(std::istream& theStream)
	{
		registerKeyword("eu5", [this](std::istream& stream) {
			eu5Building = commonItems::getString(stream);
		});
		registerKeyword("ck3", [this](std::istream& stream) {
			ck3Fragments.push_back(commonItems::getString(stream));
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string eu5Building;
	std::vector<std::string> ck3Fragments;
};
} // namespace

mappers::BuildingMapper::BuildingMapper(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::BuildingMapper::loadMappings(const std::filesystem::path& fileName)
{
	registerKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::BuildingMapper::registerKeys()
{
	registerKeyword("link", [this](std::istream& theStream) {
		const BuildingLink link(theStream);
		if (link.eu5Building.empty())
			return;
		for (const auto& fragment: link.ck3Fragments)
			fragments.emplace_back(fragment, link.eu5Building);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

std::optional<std::string> mappers::BuildingMapper::getEU5BuildingForCK3Building(const std::string& ck3Building) const
{
	for (const auto& [fragment, eu5Building]: fragments)
		if (ck3Building.find(fragment) != std::string::npos)
			return eu5Building;
	return std::nullopt;
}
