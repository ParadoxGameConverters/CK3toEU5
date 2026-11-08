#include "eu5_location_data.hpp"

#include <algorithm>
#include <string>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"

namespace
{
// game/main_menu/setup/start/06_pops.txt, relative to the EU5 install root.
const std::filesystem::path kPopsFile = std::filesystem::path("game") / "main_menu" / "setup" / "start" / "06_pops.txt";

eu5::Pop ParsePop(std::istream& input_stream)
{
   eu5::Pop pop;
   commonItems::parser pop_parser;
   pop_parser.registerKeyword("type", [&pop](std::istream& input_stream) {
      pop.type = commonItems::getString(input_stream);
   });
   pop_parser.registerKeyword("size", [&pop](std::istream& input_stream) {
      pop.size = commonItems::getDouble(input_stream);
   });
   pop_parser.registerKeyword("culture", [&pop](std::istream& input_stream) {
      pop.culture = commonItems::getString(input_stream);
   });
   pop_parser.registerKeyword("religion", [&pop](std::istream& input_stream) {
      pop.religion = commonItems::getString(input_stream);
   });
   pop_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   pop_parser.parseStream(input_stream);
   pop_parser.clearRegisteredKeywords();
   return pop;
}

// Largest total pop size wins.
std::string Dominant(const std::map<std::string, double>& totals)
{
   const auto largest = std::ranges::max_element(totals, {}, [](const auto& entry) {
      return entry.second;
   });
   return largest == totals.end() ? std::string{} : largest->first;
}
}  // namespace

eu5::LocationData::LocationData(const std::filesystem::path& eu5_directory)
{
   const auto pops_file = eu5_directory / kPopsFile;
   if (!std::filesystem::exists(pops_file))
   {
      Log(LogLevel::Warning) << "EU5 pops not found at " << pops_file.string()
                             << " - generated countries will have no culture.";
      return;
   }
   ParsePops(pops_file);
   Log(LogLevel::Info) << "<> Loaded vanilla culture and religion for " << dominant_culture_.size()
                       << " EU5 locations.";
}

void eu5::LocationData::ParsePops(const std::filesystem::path& file_path)
{
   commonItems::parser locations_parser;
   locations_parser.registerKeyword("locations", [this](std::istream& input_stream) {
      commonItems::parser location_parser;
      // Some EU5 locations carry an uppercase disambiguating suffix, such as constantine_ALG.
      location_parser.registerRegex(R"([A-Za-z0-9_']+)",
          [this](const std::string& location, std::istream& input_stream) {
             std::map<std::string, double> culture_sizes;
             std::map<std::string, double> religion_sizes;

             std::vector<eu5::Pop> location_pops;

             commonItems::parser pops_parser;
             pops_parser.registerKeyword("define_pop",
                 [&culture_sizes, &religion_sizes, &location_pops](std::istream& input_stream) {
                    const auto pop = ParsePop(input_stream);
                    if (!pop.culture.empty())
                    {
                       culture_sizes[pop.culture] += pop.size;
                    }
                    if (!pop.religion.empty())
                    {
                       religion_sizes[pop.religion] += pop.size;
                    }
                    location_pops.emplace_back(pop);
                 });
             pops_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
             pops_parser.parseStream(input_stream);
             pops_parser.clearRegisteredKeywords();

             // Locations with no pops are kept so rewriting the file preserves every entry EU5 had.
             pops_.insert_or_assign(location, std::move(location_pops));
             if (const auto culture = Dominant(culture_sizes); !culture.empty())
             {
                dominant_culture_.insert_or_assign(location, culture);
             }
             if (const auto religion = Dominant(religion_sizes); !religion.empty())
             {
                dominant_religion_.insert_or_assign(location, religion);
             }
          });
      location_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      location_parser.parseStream(input_stream);
      location_parser.clearRegisteredKeywords();
   });
   locations_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   locations_parser.parseFile(file_path);
   locations_parser.clearRegisteredKeywords();
}

std::string eu5::LocationData::GetDominantCulture(const std::string& location) const
{
   const auto culture = dominant_culture_.find(location);
   return culture == dominant_culture_.end() ? std::string{} : culture->second;
}

std::string eu5::LocationData::GetDominantReligion(const std::string& location) const
{
   const auto religion = dominant_religion_.find(location);
   return religion == dominant_religion_.end() ? std::string{} : religion->second;
}
