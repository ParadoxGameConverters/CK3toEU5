#ifndef CULTURE_MAPPER_H
#define CULTURE_MAPPER_H
#include "Parser.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mappers
{
// Maps CK3 cultures to EU5 cultures (configurables/culture_map.txt) for names the two games don't
// share, CK3 heritages to EU5 culture groups (configurables/cultureGroups_map.txt), and CK3
// languages to EU5 languages (configurables/language_map.txt).
// Together these place converted (especially dynamic) cultures into EU5's culture framework.
class CultureMapper: commonItems::parser
{
  public:
	CultureMapper() = default;

	void loadCultureMappingRules(std::istream& theStream);
	void loadCultureMappingRules(const std::filesystem::path& fileName);
	void loadCultureGroupsMappingRules(std::istream& theStream);
	void loadCultureGroupsMappingRules(const std::filesystem::path& fileName);
	void loadLanguageMappingRules(std::istream& theStream);
	void loadLanguageMappingRules(const std::filesystem::path& fileName);

	[[nodiscard]] std::optional<std::string> getEU5CultureForCK3Culture(const std::string& ck3Culture) const;
	[[nodiscard]] std::vector<std::string> getEU5GroupsForHeritage(const std::string& ck3Heritage) const;
	[[nodiscard]] std::vector<std::string> getEU5GroupsForLanguage(const std::string& ck3Language) const;
	[[nodiscard]] std::optional<std::string> getEU5LanguageForCK3Language(const std::string& ck3Language) const;
	[[nodiscard]] std::optional<std::string> getEU5LanguageForNameList(const std::string& ck3NameList) const;

	[[nodiscard]] auto getCultureMappingCount() const { return static_cast<int>(cultureToCulture.size()); }
	[[nodiscard]] auto getHeritageMappingCount() const { return static_cast<int>(heritageToGroups.size()); }
	[[nodiscard]] auto getLanguageMappingCount() const { return static_cast<int>(languageToLanguage.size()); }

  private:
	void registerCultureKeys();
	void registerCultureGroupKeys();
	void registerLanguageKeys();

	std::map<std::string, std::string> cultureToCulture;						 // ck3 culture -> eu5 culture
	std::map<std::string, std::vector<std::string>> heritageToGroups;			 // ck3 heritage -> eu5 culture groups
	std::map<std::string, std::vector<std::string>> groupLanguageToGroups;	 // ck3 language -> eu5 culture groups (from cultureGroups_map)
	std::map<std::string, std::string> languageToLanguage;						 // ck3 language -> eu5 language
	std::map<std::string, std::string> nameListToLanguage;						 // ck3 name_list -> eu5 language
};
} // namespace mappers

#endif // CULTURE_MAPPER_H
