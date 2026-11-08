#include "eu5_vanilla_countries.hpp"

#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

#include "Log.h"

namespace
{
const std::filesystem::path kCountriesFile =
    std::filesystem::path("game") / "main_menu" / "setup" / "start" / "10_countries.txt";

// Country blocks sit two levels down, inside countries = { countries = { ... } }, and the file is
// far easier to carry over verbatim than to reproduce: each block holds includes, government,
// ruler terms and centuries of ruler history that the converter has no way to regenerate.
const std::regex kCountryStart(R"(^\s*([A-Z0-9]{3})\s*=\s*\{)");

// Every bare token inside an ownership list is a location name.
void CollectLocations(const std::string& block, std::set<std::string>& locations)
{
   static const std::regex kOwnershipBlock(R"((own|control|our)[a-z_]*\s*=\s*\{([^{}]*)\})");
   for (auto match = std::sregex_iterator(block.begin(), block.end(), kOwnershipBlock); match != std::sregex_iterator();
       ++match)
   {
      std::istringstream tokens((*match)[2].str());
      std::string token;
      while (tokens >> token)
      {
         if (!token.empty() && (std::islower(static_cast<unsigned char>(token.front())) != 0))
         {
            locations.insert(token);
         }
      }
   }
}
}  // namespace

eu5::VanillaCountries::VanillaCountries(const std::filesystem::path& eu5_directory)
{
   const auto countries_file = eu5_directory / kCountriesFile;
   if (!std::filesystem::exists(countries_file))
   {
      Log(LogLevel::Warning) << "EU5 countries not found at " << countries_file.string()
                             << " - land outside CK3's map will be left unowned.";
      return;
   }
   Parse(countries_file);
   Log(LogLevel::Info) << "<> Read " << countries_.size() << " vanilla EU5 countries.";
}

void eu5::VanillaCountries::Parse(const std::filesystem::path& file_path)
{
   std::ifstream file(file_path);
   std::string line;
   while (std::getline(file, line))
   {
      std::smatch match;
      if (!std::regex_search(line, match, kCountryStart))
      {
         continue;
      }

      // Take the block verbatim, brace matching from this line onwards.
      VanillaCountry country;
      country.tag = match[1].str();
      country.block = line + "\n";

      int depth = 0;
      for (const char character: line)
      {
         depth += character == '{' ? 1 : (character == '}' ? -1 : 0);
      }
      while (depth > 0 && std::getline(file, line))
      {
         country.block += line + "\n";
         for (const char character: line)
         {
            depth += character == '{' ? 1 : (character == '}' ? -1 : 0);
         }
      }

      CollectLocations(country.block, country.locations);
      countries_.emplace_back(std::move(country));
   }
}
