#include "TraitMapper.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"

namespace
{
class TraitLink: commonItems::parser
{
  public:
	explicit TraitLink(std::istream& theStream)
	{
		registerKeyword("eu5", [this](std::istream& stream) {
			eu5Trait = commonItems::getString(stream);
		});
		registerKeyword("ck3", [this](std::istream& stream) {
			ck3Traits.push_back(commonItems::getString(stream));
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string eu5Trait;
	std::vector<std::string> ck3Traits;
};
} // namespace

mappers::TraitMapper::TraitMapper(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::TraitMapper::loadMappings(const std::filesystem::path& fileName)
{
	registerKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::TraitMapper::registerKeys()
{
	registerKeyword("link", [this](std::istream& theStream) {
		const TraitLink link(theStream);
		if (link.eu5Trait.empty())
			return;
		for (const auto& ck3Trait: link.ck3Traits)
			traitMap.emplace(ck3Trait, link.eu5Trait);
	});
	registerKeyword("general", [this](std::istream& theStream) {
		const TraitLink link(theStream);
		if (link.eu5Trait.empty())
			return;
		for (const auto& ck3Trait: link.ck3Traits)
			generalTraitMap.emplace(ck3Trait, link.eu5Trait);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

std::optional<std::string> mappers::TraitMapper::getEU5TraitForCK3Trait(const std::string& ck3Trait) const
{
	if (const auto& match = traitMap.find(ck3Trait); match != traitMap.end())
		return match->second;
	return std::nullopt;
}

std::optional<std::string> mappers::TraitMapper::getGeneralTraitForCK3Trait(const std::string& ck3Trait) const
{
	if (const auto& match = generalTraitMap.find(ck3Trait); match != generalTraitMap.end())
		return match->second;
	return std::nullopt;
}
