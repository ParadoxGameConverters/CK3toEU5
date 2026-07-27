#include "TitleTagMapper.h"
#include "CommonRegexes.h"
#include "Log.h"
#include "ParserHelpers.h"
#include <format>

namespace
{
class TagMappingLink: commonItems::parser
{
  public:
	explicit TagMappingLink(std::istream& theStream)
	{
		registerKeyword("ck3", [this](std::istream& stream) {
			ck3Title = commonItems::getString(stream);
		});
		registerKeyword("eu5", [this](std::istream& stream) {
			eu5Tag = commonItems::getString(stream);
		});
		registerKeyword("capitals", [this](std::istream& stream) {
			for (const auto& location: commonItems::getStrings(stream))
				capitals.emplace_back(location);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string ck3Title;
	std::string eu5Tag;
	std::vector<std::string> capitals;
};
} // namespace

mappers::TitleTagMapper::TitleTagMapper(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::TitleTagMapper::loadMappings(const std::filesystem::path& fileName)
{
	registerKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::TitleTagMapper::registerKeys()
{
	registerKeyword("link", [this](std::istream& theStream) {
		const TagMappingLink link(theStream);
		if (link.eu5Tag.empty())
			return;
		usedTags.insert(link.eu5Tag);
		if (!link.ck3Title.empty())
			titleToTag.emplace(link.ck3Title, link.eu5Tag);
		for (const auto& capital: link.capitals)
			capitalToTag.emplace(capital, link.eu5Tag);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

void mappers::TitleTagMapper::registerTag(const std::string& tag)
{
	usedTags.insert(tag);
}

std::optional<std::string> mappers::TitleTagMapper::getTagForTitle(const std::string& ck3Title)
{
	return getTagForTitle(ck3Title, std::string());
}

std::optional<std::string> mappers::TitleTagMapper::getTagForTitle(const std::string& ck3Title, const std::string& eu5CapitalLocation)
{
	if (ck3Title.empty())
		return std::nullopt;

	// Did we already decide on this title?
	if (const auto& cached = assignedTitleTags.find(ck3Title); cached != assignedTitleTags.end())
		return cached->second;

	// Capitals take precedence over direct title mappings.
	if (!eu5CapitalLocation.empty())
	{
		if (const auto& capitalMatch = capitalToTag.find(eu5CapitalLocation); capitalMatch != capitalToTag.end())
		{
			assignedTitleTags.emplace(ck3Title, capitalMatch->second);
			return capitalMatch->second;
		}
	}

	if (const auto& titleMatch = titleToTag.find(ck3Title); titleMatch != titleToTag.end())
	{
		assignedTitleTags.emplace(ck3Title, titleMatch->second);
		return titleMatch->second;
	}

	// No mapping. Generate a dynamic tag.
	const auto newTag = generateNewTag();
	assignedTitleTags.emplace(ck3Title, newTag);
	return newTag;
}

std::string mappers::TitleTagMapper::generateNewTag()
{
	// Tags must stay 3 characters: Z00-Z99, then Y00-Y99 and so on down the alphabet.
	std::string tag;
	do
	{
		const auto letter = static_cast<char>('Z' - generatedTagCounter / 100 % 26);
		tag = std::format("{}{:02d}", letter, generatedTagCounter % 100);
		++generatedTagCounter;
	} while (usedTags.contains(tag));
	usedTags.insert(tag);
	return tag;
}
