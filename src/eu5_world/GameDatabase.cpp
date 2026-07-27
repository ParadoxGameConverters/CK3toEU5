#include "GameDatabase.h"
#include "CommonRegexes.h"
#include "Log.h"
#include "OSCompatibilityLayer.h"
#include "ParserHelpers.h"
#include "src/output/BlockParsing.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>

namespace
{
class ReligionEntry: commonItems::parser
{
  public:
	explicit ReligionEntry(std::istream& theStream)
	{
		registerKeyword("group", [this](std::istream& stream) {
			group = commonItems::getString(stream);
		});
		registerKeyword("language", [this](std::istream& stream) {
			language = commonItems::getString(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string group;
	std::string language;
};

class CultureEntry: commonItems::parser
{
  public:
	explicit CultureEntry(std::istream& theStream)
	{
		registerKeyword("language", [this](std::istream& stream) {
			language = commonItems::getString(stream);
		});
		registerKeyword("culture_groups", [this](std::istream& stream) {
			groups = commonItems::getStrings(stream);
		});
		registerKeyword("tags", [this](std::istream& stream) {
			gfxTags = commonItems::getStrings(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string language;
	std::vector<std::string> groups;
	std::vector<std::string> gfxTags;
};
} // namespace

void EU5::GameDatabase::loadCultures(const std::filesystem::path& culturesFolder)
{
	for (const auto& file: commonItems::GetAllFilesInFolder(culturesFolder))
	{
		if (file.extension() != ".txt")
			continue;
		std::ifstream theFile(culturesFolder / file);
		if (!theFile.is_open())
			continue;
		commonItems::absorbBOM(theFile);
		loadCulturesFromStream(theFile);
		theFile.close();
	}
}

void EU5::GameDatabase::loadCulturesFromStream(std::istream& theStream)
{
	commonItems::parser cultureParser;
	cultureParser.registerRegex(commonItems::catchallRegex, [this](const std::string& cultureName, std::istream& stream) {
		cultures.insert(cultureName);
		const CultureEntry entry(stream);
		cultureLanguages[cultureName] = entry.language;
		cultureGroups[cultureName] = entry.groups;
		cultureGfxTags[cultureName] = entry.gfxTags;
		for (const auto& group: entry.groups)
			groupCultures[group].push_back(cultureName);
	});
	cultureParser.parseStream(theStream);
}

void EU5::GameDatabase::loadReligions(const std::filesystem::path& religionsFolder)
{
	for (const auto& file: commonItems::GetAllFilesInFolder(religionsFolder))
	{
		if (file.extension() != ".txt")
			continue;
		std::ifstream theFile(religionsFolder / file);
		if (!theFile.is_open())
			continue;
		commonItems::absorbBOM(theFile);
		loadReligionsFromStream(theFile);
		theFile.close();
	}
}

void EU5::GameDatabase::loadReligionsFromStream(std::istream& theStream)
{
	commonItems::parser religionParser;
	religionParser.registerRegex(commonItems::catchallRegex, [this](const std::string& religionName, std::istream& stream) {
		const ReligionEntry entry(stream);
		religionGroups[religionName] = entry.group;
		if (!entry.language.empty())
			religionLanguages[religionName] = entry.language;
	});
	religionParser.parseStream(theStream);
}

namespace
{
class LanguageEntry: commonItems::parser
{
  public:
	explicit LanguageEntry(std::istream& theStream)
	{
		registerKeyword("dialects", [this](std::istream& stream) {
			commonItems::parser dialectParser;
			dialectParser.registerRegex(commonItems::catchallRegex, [this](const std::string& dialectName, std::istream& dialectStream) {
				commonItems::ignoreItem(dialectName, dialectStream);
				dialects.push_back(dialectName);
			});
			dialectParser.parseStream(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::vector<std::string> dialects;
};
} // namespace

void EU5::GameDatabase::loadLanguages(const std::filesystem::path& languagesFolder)
{
	for (const auto& file: commonItems::GetAllFilesInFolder(languagesFolder))
	{
		if (file.extension() != ".txt")
			continue;
		std::ifstream theFile(languagesFolder / file);
		if (!theFile.is_open())
			continue;
		commonItems::absorbBOM(theFile);
		loadLanguagesFromStream(theFile);
		theFile.close();
	}
}

void EU5::GameDatabase::loadLanguagesFromStream(std::istream& theStream)
{
	commonItems::parser languageParser;
	languageParser.registerRegex(commonItems::catchallRegex, [this](const std::string& languageName, std::istream& stream) {
		const LanguageEntry entry(stream);
		languageDialects[languageName] = entry.dialects;
	});
	languageParser.parseStream(theStream);
}

std::string EU5::GameDatabase::resolveLanguage(const std::string& language) const
{
	if (language.empty())
		return language;
	const auto& match = languageDialects.find(language);
	if (match == languageDialects.end())
		return language; // a dialect key, or something we never saw - nothing to resolve against.
	const auto& dialects = match->second;
	if (dialects.empty() || std::ranges::find(dialects, language) != dialects.end())
		return language; // referenceable as-is.
	// The dialect most of the language's vanilla cultures speak stands in for the language itself.
	std::string best;
	auto bestCount = -1;
	for (const auto& dialect: dialects)
	{
		auto count = 0;
		for (const auto& [culture, cultureLanguage]: cultureLanguages)
			if (cultureLanguage == dialect)
				++count;
		if (count > bestCount)
		{
			best = dialect;
			bestCount = count;
		}
	}
	return best;
}

void EU5::GameDatabase::loadSetupTemplates(const std::filesystem::path& templatesFolder)
{
	// Templates are plain script; the only thing needed here is the societal values they set, which
	// are the baseline a converted country's ethos and traditions push against.
	static const std::regex valueLine(R"((\w+_vs_\w+)\s*=\s*(-?\d+))");
	for (const auto& file: commonItems::GetAllFilesInFolder(templatesFolder))
	{
		if (file.extension() != ".txt")
			continue;
		std::ifstream theFile(templatesFolder / file);
		if (!theFile.is_open())
			continue;
		commonItems::absorbBOM(theFile);
		const std::string text((std::istreambuf_iterator<char>(theFile)), std::istreambuf_iterator<char>());
		theFile.close();
		const auto name = file.stem().string();
		auto& values = templateValues[name];
		for (auto match = std::sregex_iterator(text.begin(), text.end(), valueLine); match != std::sregex_iterator(); ++match)
			values[(*match)[1].str()] = std::stoi((*match)[2].str());

		// The laws and privileges the template hands out. Both are gated on technology, so a country
		// including this template has to be advanced enough for everything in it.
		auto& setup = templateSetups[name];
		static const std::regex lawLine(R"((\w+)\s*=\s*(\w+))");
		const auto laws = output::extractBlockBody(text, "laws");
		for (auto match = std::sregex_iterator(laws.begin(), laws.end(), lawLine); match != std::sregex_iterator(); ++match)
			setup.laws.insert((*match)[1].str());
		static const std::regex wordLine(R"(\w+)");
		const auto privileges = output::extractBlockBody(text, "privilege");
		for (auto match = std::sregex_iterator(privileges.begin(), privileges.end(), wordLine); match != std::sregex_iterator(); ++match)
			setup.privileges.insert(match->str());
	}
}

namespace
{
// Strips a name down to bare lowercase letters so "Abd al-Rahman", "abd_al_rahman" and
// "Abd-al-Rahmān" all land on the same lookup.
std::string normalizeName(const std::string& name)
{
	std::string normalized;
	for (const unsigned char c: name)
		if (std::isalpha(c))
			normalized += static_cast<char>(std::tolower(c));
	return normalized;
}
} // namespace

void EU5::GameDatabase::loadCharacterNames(const std::filesystem::path& nameLocFile)
{
	// EU5 numbers monarchs by name_* keys, and the localization is the only place those keys are
	// written out as readable names. Both sides are indexed: the key's own suffix ("name_henry" ->
	// "henry") and every localized form of it ("Henrik", "Harri"), so a CK3 name matches whichever
	// spelling the game happens to use.
	std::ifstream input(nameLocFile);
	if (!input.is_open())
	{
		Log(LogLevel::Warning) << "Could not read EU5 character names from " << nameLocFile.string() << "; regnal numbering will be shallow.";
		return;
	}
	commonItems::absorbBOM(input);
	static const std::regex entry(R"(^\s*(name_[a-z0-9_]+)(\.[\w.]+)?:\s*\d*\s*\"([^\"]*)\")");
	std::string line;
	while (std::getline(input, line))
	{
		std::smatch match;
		if (!std::regex_search(line, match, entry))
			continue;
		const auto& key = match[1].str();
		nameKeys.emplace(normalizeName(key.substr(5)), key);
		if (const auto localized = normalizeName(match[3].str()); !localized.empty())
			nameKeys.emplace(localized, key);
	}
	Log(LogLevel::Info) << "<> " << nameKeys.size() << " monarch name spellings recognized.";
}

std::string EU5::GameDatabase::getNameKey(const std::string& firstName) const
{
	if (const auto& match = nameKeys.find(normalizeName(firstName)); match != nameKeys.end())
		return match->second;
	return {};
}

std::map<std::string, int> EU5::GameDatabase::getTemplateValues(const std::string& templateName) const
{
	if (const auto& match = templateValues.find(templateName); match != templateValues.end())
		return match->second;
	return {};
}

std::string EU5::GameDatabase::getReligionGroup(const std::string& religion) const
{
	if (const auto& match = religionGroups.find(religion); match != religionGroups.end())
		return match->second;
	return {};
}

std::string EU5::GameDatabase::getReligionLanguage(const std::string& religion) const
{
	if (const auto& match = religionLanguages.find(religion); match != religionLanguages.end())
		return match->second;
	return {};
}

std::vector<std::string> EU5::GameDatabase::getReligionsInGroup(const std::string& group) const
{
	std::vector<std::string> religions;
	for (const auto& [religion, religionGroup]: religionGroups)
		if (religionGroup == group)
			religions.push_back(religion);
	return religions;
}

std::string EU5::GameDatabase::getCultureLanguage(const std::string& culture) const
{
	if (const auto& match = cultureLanguages.find(culture); match != cultureLanguages.end())
		return match->second;
	return {};
}

std::vector<std::string> EU5::GameDatabase::getCultureGfxTags(const std::string& culture) const
{
	if (const auto& match = cultureGfxTags.find(culture); match != cultureGfxTags.end())
		return match->second;
	return {};
}

std::vector<std::string> EU5::GameDatabase::getCulturesInGroup(const std::string& group) const
{
	if (const auto& match = groupCultures.find(group); match != groupCultures.end())
		return match->second;
	return {};
}

bool EU5::GameDatabase::isCultureInGroup(const std::string& culture, const std::string& group) const
{
	const auto& match = cultureGroups.find(culture);
	if (match == cultureGroups.end())
		return false;
	return std::find(match->second.begin(), match->second.end(), group) != match->second.end();
}

bool EU5::GameDatabase::sharesCultureGroup(const std::string& first, const std::string& second) const
{
	if (first.empty() || second.empty())
		return false;
	if (first == second)
		return true;
	const auto& firstGroups = cultureGroups.find(first);
	const auto& secondGroups = cultureGroups.find(second);
	if (firstGroups == cultureGroups.end() || secondGroups == cultureGroups.end())
		return false;
	for (const auto& group: firstGroups->second)
		if (std::find(secondGroups->second.begin(), secondGroups->second.end(), group) != secondGroups->second.end())
			return true;
	return false;
}
