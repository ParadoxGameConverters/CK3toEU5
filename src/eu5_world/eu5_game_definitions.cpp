#include "eu5_game_definitions.hpp"

#include <string>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"

namespace
{
const std::filesystem::path kCountriesFolder = std::filesystem::path("game") / "in_game" / "setup" / "countries";
const std::filesystem::path kCulturesFolder = std::filesystem::path("game") / "in_game" / "common" / "cultures";
const std::filesystem::path kReligionsFolder = std::filesystem::path("game") / "in_game" / "common" / "religions";

const std::string kTagPattern = R"([A-Z0-9]{3})";
const std::string kKeyPattern = R"([a-z_0-9]+)";
}  // namespace

eu5::GameDefinitions::GameDefinitions(const std::filesystem::path& eu5_directory)
{
   LoadKeys(eu5_directory / kCountriesFolder, tags_, kTagPattern);
   LoadCultures(eu5_directory / kCulturesFolder);
   LoadKeys(eu5_directory / kReligionsFolder, religions_, kKeyPattern);

   if (!IsLoaded())
   {
      Log(LogLevel::Warning) << "Could not read EU5 definitions from " << eu5_directory.string()
                             << " - converted output cannot be validated against the game.";
      return;
   }
   Log(LogLevel::Info) << "<> EU5 defines " << tags_.size() << " country tags, " << cultures_.size() << " cultures in "
                       << culture_groups_.size() << " groups speaking " << languages_.size() << " languages, and "
                       << religions_.size() << " religions.";
}

const eu5::CultureDefinition* eu5::GameDefinitions::GetCultureDefinition(const std::string& culture) const
{
   const auto definition = culture_definitions_.find(culture);
   if (definition == culture_definitions_.end())
   {
      return nullptr;
   }
   return &definition->second;
}

void eu5::GameDefinitions::LoadKeys(const std::filesystem::path& folder,
    std::set<std::string>& target,
    const std::string& pattern)
{
   if (!std::filesystem::exists(folder))
   {
      return;
   }
   for (const auto& entry: std::filesystem::directory_iterator(folder))
   {
      if (!entry.is_regular_file() || entry.path().extension() != ".txt")
      {
         continue;
      }
      commonItems::parser parser;
      parser.registerRegex(pattern, [&target](const std::string& key, std::istream& input_stream) {
         target.insert(key);
         commonItems::ignoreItem(key, input_stream);
      });
      parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      parser.parseFile(entry.path());
      parser.clearRegisteredKeywords();
   }
}

void eu5::GameDefinitions::LoadCultures(const std::filesystem::path& folder)
{
   if (!std::filesystem::exists(folder))
   {
      return;
   }

   commonItems::parser culture_parser;
   CultureDefinition* current = nullptr;
   culture_parser.registerKeyword("language", [&current](std::istream& input_stream) {
      const auto language = commonItems::getString(input_stream);
      if (current != nullptr)
      {
         current->language = language;
      }
   });
   culture_parser.registerKeyword("culture_groups", [&current](std::istream& input_stream) {
      const auto groups = commonItems::getStrings(input_stream);
      if (current != nullptr)
      {
         current->groups = groups;
      }
   });
   culture_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

   for (const auto& entry: std::filesystem::directory_iterator(folder))
   {
      if (!entry.is_regular_file() || entry.path().extension() != ".txt")
      {
         continue;
      }
      commonItems::parser parser;
      // EU5 is inconsistent about the suffix - english and dakelh_culture are both top level
      // culture keys - so the key itself cannot be used to tell a culture from anything else.
      parser.registerRegex(kKeyPattern, [this, &culture_parser, &current](const std::string& key,
                                            std::istream& input_stream) {
         cultures_.insert(key);
         current = &culture_definitions_[key];
         culture_parser.parseStream(input_stream);
         current = nullptr;
      });
      parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      parser.parseFile(entry.path());
      parser.clearRegisteredKeywords();
   }

   for (const auto& [name, definition]: culture_definitions_)
   {
      if (!definition.language.empty())
      {
         languages_.insert(definition.language);
      }
      culture_groups_.insert(definition.groups.begin(), definition.groups.end());
   }
}
