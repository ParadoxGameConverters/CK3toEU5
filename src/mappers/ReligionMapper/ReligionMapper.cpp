#include "ReligionMapper.h"
#include "CommonRegexes.h"
#include "Log.h"
#include "ParserHelpers.h"
#include <vector>

namespace
{
class ReligionMappingLink: commonItems::parser
{
  public:
	explicit ReligionMappingLink(std::istream& theStream)
	{
		registerKeyword("eu5", [this](std::istream& stream) {
			eu5Religion = commonItems::getString(stream);
		});
		registerKeyword("ck3", [this](std::istream& stream) {
			ck3Faiths.push_back(commonItems::getString(stream));
		});
		registerKeyword("religious_head", [this](std::istream& stream) {
			religiousHeads.push_back(commonItems::getString(stream));
		});
		registerKeyword("school", [this](std::istream& stream) {
			school = commonItems::getString(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string eu5Religion;
	std::vector<std::string> ck3Faiths;
	std::vector<std::string> religiousHeads;
	std::optional<std::string> school;
};
} // namespace

mappers::ReligionMapper::ReligionMapper(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::ReligionMapper::loadMappings(const std::filesystem::path& fileName)
{
	registerKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::ReligionMapper::registerKeys()
{
	registerKeyword("link", [this](std::istream& theStream) {
		const ReligionMappingLink link(theStream);
		if (link.eu5Religion.empty())
			return;
		const ReligionMapping mapping{link.eu5Religion, link.school};
		for (const auto& faith: link.ck3Faiths)
			faithToReligion.emplace(faith, mapping);
		for (const auto& head: link.religiousHeads)
			religiousHeadToReligion.emplace(head, mapping);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

std::optional<mappers::ReligionMapping> mappers::ReligionMapper::getEU5ReligionForCK3Faith(const std::string& ck3Faith,
	 const std::string& religiousHeadTitle) const
{
	if (!religiousHeadTitle.empty())
	{
		if (const auto& headMatch = religiousHeadToReligion.find(religiousHeadTitle); headMatch != religiousHeadToReligion.end())
			return headMatch->second;
	}
	if (const auto& faithMatch = faithToReligion.find(ck3Faith); faithMatch != faithToReligion.end())
		return faithMatch->second;
	return std::nullopt;
}
