#ifndef EU5_GAME_DEFINITIONS_H
#define EU5_GAME_DEFINITIONS_H

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace eu5
{

// An EU5 culture as the game itself defines it. A definition carries a good deal more, but the
// language and the groups are what decides whether a CK3 culture and an EU5 one are the same
// people described at different resolutions.
struct CultureDefinition
{
   std::string language;
   std::vector<std::string> groups;
};

// What EU5 actually defines: country tags, cultures and religions, scraped from the install.
//
// The configurables carry a lot of EU4 era names that EU5 renamed or never had - the Ottomans are
// TUR rather than OTT, Shia is shia rather than shiite - and writing one into the output makes EU5
// reject the block it appears in. A rejected block near the top of 10_countries.txt takes the rest
// of the file with it, silently, so everything the converter emits is checked against this first.
class GameDefinitions
{
  public:
   GameDefinitions() = default;
   explicit GameDefinitions(const std::filesystem::path& eu5_directory);

   [[nodiscard]] bool HasTag(const std::string& tag) const { return tags_.contains(tag); }
   [[nodiscard]] bool HasCulture(const std::string& culture) const { return cultures_.contains(culture); }
   [[nodiscard]] bool HasReligion(const std::string& religion) const { return religions_.contains(religion); }
   [[nodiscard]] bool HasLanguage(const std::string& language) const { return languages_.contains(language); }
   [[nodiscard]] bool HasCultureGroup(const std::string& group) const { return culture_groups_.contains(group); }

   [[nodiscard]] const auto& GetTags() const { return tags_; }
   [[nodiscard]] const auto& GetCultures() const { return cultures_; }
   [[nodiscard]] const auto& GetReligions() const { return religions_; }
   [[nodiscard]] const auto& GetLanguages() const { return languages_; }
   [[nodiscard]] const auto& GetCultureGroups() const { return culture_groups_; }
   [[nodiscard]] const auto& GetCultureDefinitions() const { return culture_definitions_; }

   // Null when EU5 does not define this culture.
   [[nodiscard]] const CultureDefinition* GetCultureDefinition(const std::string& culture) const;

   // False when the EU5 install could not be read, in which case nothing can be validated.
   [[nodiscard]] bool IsLoaded() const { return !tags_.empty(); }

  private:
   // Collects the top level keys of every .txt in the folder.
   void LoadKeys(const std::filesystem::path& folder, std::set<std::string>& target, const std::string& pattern);
   // Culture definitions need their contents as well as their names.
   void LoadCultures(const std::filesystem::path& folder);

   std::set<std::string> tags_;
   std::set<std::string> cultures_;
   std::set<std::string> religions_;
   std::set<std::string> languages_;
   std::set<std::string> culture_groups_;
   std::map<std::string, CultureDefinition> culture_definitions_;
};

}  // namespace eu5

#endif  // EU5_GAME_DEFINITIONS_H
