#include "CultureMapper.h"
#include "CommonRegexes.h"
#include "Log.h"
#include "ParserHelpers.h"

namespace
{
class CultureGroupLink: commonItems::parser
{
  public:
	explicit CultureGroupLink(std::istream& theStream)
	{
		registerKeyword("eu5", [this](std::istream& stream) {
			eu5Groups.push_back(commonItems::getString(stream));
		});
		registerKeyword("heritage", [this](std::istream& stream) {
			heritage = commonItems::getString(stream);
		});
		registerKeyword("language", [this](std::istream& stream) {
			language = commonItems::getString(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::vector<std::string> eu5Groups;
	std::string heritage;
	std::string language;
};

class CultureLink: commonItems::parser
{
  public:
	explicit CultureLink(std::istream& theStream)
	{
		registerKeyword("eu5", [this](std::istream& stream) {
			eu5Culture = commonItems::getString(stream);
		});
		registerKeyword("ck3", [this](std::istream& stream) {
			ck3Cultures.push_back(commonItems::getString(stream));
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string eu5Culture;
	std::vector<std::string> ck3Cultures;
};

class LanguageLink: commonItems::parser
{
  public:
	explicit LanguageLink(std::istream& theStream)
	{
		registerKeyword("eu5", [this](std::istream& stream) {
			eu5Language = commonItems::getString(stream);
		});
		registerKeyword("ck3", [this](std::istream& stream) {
			ck3Languages.push_back(commonItems::getString(stream));
		});
		registerKeyword("name_list", [this](std::istream& stream) {
			nameLists.push_back(commonItems::getString(stream));
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string eu5Language;
	std::vector<std::string> ck3Languages;
	std::vector<std::string> nameLists;
};
} // namespace

void mappers::CultureMapper::loadCultureMappingRules(std::istream& theStream)
{
	registerCultureKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::CultureMapper::loadCultureMappingRules(const std::filesystem::path& fileName)
{
	registerCultureKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::CultureMapper::loadCultureGroupsMappingRules(std::istream& theStream)
{
	registerCultureGroupKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::CultureMapper::loadCultureGroupsMappingRules(const std::filesystem::path& fileName)
{
	registerCultureGroupKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::CultureMapper::loadLanguageMappingRules(std::istream& theStream)
{
	registerLanguageKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::CultureMapper::loadLanguageMappingRules(const std::filesystem::path& fileName)
{
	registerLanguageKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::CultureMapper::registerCultureKeys()
{
	registerKeyword("link", [this](std::istream& theStream) {
		const CultureLink link(theStream);
		if (link.eu5Culture.empty())
			return;
		for (const auto& culture: link.ck3Cultures)
			cultureToCulture.emplace(culture, link.eu5Culture);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

void mappers::CultureMapper::registerCultureGroupKeys()
{
	registerKeyword("link", [this](std::istream& theStream) {
		const CultureGroupLink link(theStream);
		if (link.eu5Groups.empty())
			return;
		if (!link.heritage.empty())
			heritageToGroups.emplace(link.heritage, link.eu5Groups);
		if (!link.language.empty())
			groupLanguageToGroups.emplace(link.language, link.eu5Groups);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

void mappers::CultureMapper::registerLanguageKeys()
{
	registerKeyword("link", [this](std::istream& theStream) {
		const LanguageLink link(theStream);
		if (link.eu5Language.empty())
			return;
		for (const auto& language: link.ck3Languages)
			languageToLanguage.emplace(language, link.eu5Language);
		for (const auto& nameList: link.nameLists)
			nameListToLanguage.emplace(nameList, link.eu5Language);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

std::optional<std::string> mappers::CultureMapper::getEU5CultureForCK3Culture(const std::string& ck3Culture) const
{
	if (const auto& match = cultureToCulture.find(ck3Culture); match != cultureToCulture.end())
		return match->second;
	return std::nullopt;
}

std::vector<std::string> mappers::CultureMapper::getEU5GroupsForHeritage(const std::string& ck3Heritage) const
{
	if (const auto& match = heritageToGroups.find(ck3Heritage); match != heritageToGroups.end())
		return match->second;
	return {};
}

std::vector<std::string> mappers::CultureMapper::getEU5GroupsForLanguage(const std::string& ck3Language) const
{
	if (const auto& match = groupLanguageToGroups.find(ck3Language); match != groupLanguageToGroups.end())
		return match->second;
	return {};
}

std::optional<std::string> mappers::CultureMapper::getEU5LanguageForCK3Language(const std::string& ck3Language) const
{
	if (const auto& match = languageToLanguage.find(ck3Language); match != languageToLanguage.end())
		return match->second;
	return std::nullopt;
}

std::optional<std::string> mappers::CultureMapper::getEU5LanguageForNameList(const std::string& ck3NameList) const
{
	if (const auto& match = nameListToLanguage.find(ck3NameList); match != nameListToLanguage.end())
		return match->second;
	return std::nullopt;
}
