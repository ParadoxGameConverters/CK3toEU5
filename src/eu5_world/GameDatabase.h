#ifndef EU5_GAME_DATABASE_H
#define EU5_GAME_DATABASE_H
#include "Parser.h"
#include <map>
#include <set>
#include <string>
#include <vector>

namespace EU5
{
// Scans EU5's in_game/common databases we need for validation:
// cultures (in_game/common/cultures/*.txt) and religions with their groups (in_game/common/religions/*.txt).
class GameDatabase
{
  public:
	GameDatabase() = default;

	void loadCultures(const std::filesystem::path& culturesFolder);
	void loadCulturesFromStream(std::istream& theStream);
	void loadReligions(const std::filesystem::path& religionsFolder);
	void loadReligionsFromStream(std::istream& theStream);
	void loadLanguages(const std::filesystem::path& languagesFolder);
	void loadLanguagesFromStream(std::istream& theStream);
	void loadSetupTemplates(const std::filesystem::path& templatesFolder);
	void loadCharacterNames(const std::filesystem::path& nameLocFile);

	// The game only resolves language references to leaves: a dialect, or a language without
	// dialects (or one listing itself as its own dialect, like greek_language). Anything else
	// fails to parse and takes the surrounding block down with it. Given a language, this returns
	// something the game will accept - the language itself when referenceable, otherwise the
	// dialect vanilla cultures use most (first declared on a tie).
	[[nodiscard]] std::string resolveLanguage(const std::string& language) const;

	// The name_* key EU5 numbers monarchs by, for a given first name. Empty if the game has no such name.
	[[nodiscard]] std::string getNameKey(const std::string& firstName) const;

	// The societal values a setup template starts a country on, so shifts have something to shift.
	[[nodiscard]] std::map<std::string, int> getTemplateValues(const std::string& templateName) const;
	// The laws and estate privileges a setup template hands its countries, both gated on technology.
	[[nodiscard]] const auto& getTemplateSetups() const { return templateSetups; }
	[[nodiscard]] bool hasTemplate(const std::string& templateName) const { return templateValues.contains(templateName); }

	[[nodiscard]] bool isValidCulture(const std::string& culture) const { return cultures.contains(culture); }
	[[nodiscard]] bool isValidReligion(const std::string& religion) const { return religionGroups.contains(religion); }
	[[nodiscard]] std::string getReligionGroup(const std::string& religion) const;
	[[nodiscard]] std::string getReligionLanguage(const std::string& religion) const;
	[[nodiscard]] std::vector<std::string> getReligionsInGroup(const std::string& group) const;
	[[nodiscard]] std::string getCultureLanguage(const std::string& culture) const;
	[[nodiscard]] std::vector<std::string> getCultureGfxTags(const std::string& culture) const;
	[[nodiscard]] std::vector<std::string> getCulturesInGroup(const std::string& group) const;
	[[nodiscard]] bool isCultureInGroup(const std::string& culture, const std::string& group) const;
	// Whether two cultures are close relatives - same culture group. Kin enough that a realm ruling
	// one is not ruling the other as a foreign occupier.
	[[nodiscard]] bool sharesCultureGroup(const std::string& first, const std::string& second) const;

	[[nodiscard]] auto getCultureCount() const { return static_cast<int>(cultures.size()); }
	[[nodiscard]] auto getLanguageCount() const { return static_cast<int>(languageDialects.size()); }
	[[nodiscard]] auto getReligionCount() const { return static_cast<int>(religionGroups.size()); }
	[[nodiscard]] auto getTemplateCount() const { return static_cast<int>(templateValues.size()); }
	[[nodiscard]] auto getNameKeyCount() const { return static_cast<int>(nameKeys.size()); }

	struct TemplateSetup
	{
		std::set<std::string> laws;
		std::set<std::string> privileges;
	};

  private:
	std::set<std::string> cultures;
	std::map<std::string, std::vector<std::string>> languageDialects;		  // language -> its declared dialects, in file order
	std::map<std::string, std::string> cultureLanguages;						  // culture -> language
	std::map<std::string, std::vector<std::string>> cultureGroups;			  // culture -> groups
	std::map<std::string, std::vector<std::string>> cultureGfxTags;			  // culture -> graphical culture tags
	std::map<std::string, std::vector<std::string>> groupCultures;			  // group -> cultures
	std::map<std::string, std::string> religionGroups;							  // religion -> group
	std::map<std::string, std::string> religionLanguages;						  // religion -> liturgical language
	std::map<std::string, std::map<std::string, int>> templateValues;		  // setup template -> (societal value -> position)
	std::map<std::string, TemplateSetup> templateSetups;						  // setup template -> the laws and privileges it grants
	std::map<std::string, std::string> nameKeys;									  // normalized first name -> name_* key
};
} // namespace EU5

#endif // EU5_GAME_DATABASE_H
