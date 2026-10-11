#include "countries_file.hpp"

#include <external/commonItems/Log.h>

#include <algorithm>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "src/ck3_world/realms/realm.hpp"
#include "src/eu5_world/eu5_country.hpp"

namespace
{
// Vanilla's own 10_countries.txt wraps the blocks in an outer countries = { countries = { ... } }
// and keeps the location lists to readable line lengths.
constexpr int kLocationsPerLine = 8;

// What vanilla starts in. EU5 has a single start date, so there is no other age to choose.
const std::string kCurrentAge = "age_1_traditions";

// CK3's government types onto the ones EU5 accepts in a country's government block.
std::string GovernmentFor(const std::string& ck3_government)
{
   if (ck3_government.starts_with("tribal"))
   {
      return "tribe";
   }
   if (ck3_government.starts_with("theocracy"))
   {
      return "theocracy";
   }
   if (ck3_government.starts_with("republic"))
   {
      return "republic";
   }
   // Feudal and clan both sit closest to a monarchy.
   return "monarchy";
}

std::string ParliamentFor(const std::string& government)
{
   if (government == "tribe")
   {
      return "assembly";
   }
   if (government == "republic")
   {
      return "estate_parliament";
   }
   return "council";
}

// The axes CK3 says nothing about. Writing them neutral is honest and stops EU5 complaining that
// the country has no society values scripted.
const std::vector<std::string> kNeutralSocietyAxes = {"spiritualist_vs_humanist",
    "aristocracy_vs_plutocracy",
    "serfdom_vs_free_subjects",
    "mercantilism_vs_free_trade",
    "belligerent_vs_conciliatory",
    "quality_vs_quantity",
    "offensive_vs_defensive",
    "land_vs_naval",
    "capital_economy_vs_traditional_economy",
    "individualism_vs_communalism",
    "outward_vs_inward"};

bool ShouldWrite(const eu5::Country& country)
{
   if (country.GetLocations().empty())
   {
      return false;
   }
   // A tag EU5 does not define needs one written alongside this file. Where that could not be
   // generated the tag would be rejected, and a rejected block takes the rest of the file with it,
   // so the country is left out entirely.
   return !country.NeedsDefinition() || (country.GetCulture().has_value() && country.GetReligion().has_value());
}

void WriteGovernment(std::ostringstream& output, const eu5::Country& country)
{
   const auto government = GovernmentFor(country.GetSourceRealm()->GetGovernment());
   output << "\t\t\tgovernment = {\n";
   output << "\t\t\t\ttype = " << government << "\n";
   if (country.HasRuler())
   {
      output << "\t\t\t\truler = " << country.GetRulerId() << "\n";
   }
   output << "\t\t\t\tparliament = { parliament_type = " << ParliamentFor(government) << " }\n";
   // EU5 wants every country placed on its society axes and complains for each one that is not.
   // CK3 has no equivalent for most of them, so only the two its government type genuinely speaks
   // to are leaned; the rest sit neutral rather than inventing a position.
   output << "\t\t\t\tcentralization_vs_decentralization = " << (government == "tribe" ? 40 : -20) << "\n";
   output << "\t\t\t\ttraditionalist_vs_innovative = " << (government == "tribe" ? -40 : -20) << "\n";
   for (const auto& axis: kNeutralSocietyAxes)
   {
      output << "\t\t\t\t" << axis << " = 0\n";
   }
   output << "\t\t\t}\n\n";
}

void WriteLocations(std::ostringstream& output, const eu5::Country& country)
{
   output << "\t\t\town_control_core = {\n";
   int on_this_line = 0;
   for (const auto& location: country.GetLocations())
   {
      if (on_this_line == 0)
      {
         output << "\t\t\t\t";
      }
      output << location << " ";
      if (++on_this_line == kLocationsPerLine)
      {
         output << "\n";
         on_this_line = 0;
      }
   }
   if (on_this_line != 0)
   {
      output << "\n";
   }
   output << "\t\t\t}\n";
}

void WriteCountry(std::ostringstream& output, const eu5::Country& country)
{
   output << "\n\t\t" << country.GetTag() << " = { # " << country.GetSourceRealm()->GetRealmName() << "\n";
   output << "\t\t\tcountry_rank = " << country.GetRank() << "\n";
   output << "\t\t\tstarting_technology_level = " << country.GetTechnologyLevel() << "\n\n";
   WriteGovernment(output, country);
   WriteLocations(output, country);
   output << "\t\t}\n";
}

// Everything CK3 does not cover - the Americas, Oceania, much of Siberia - would otherwise be left
// with no owner at all, since this file replaces EU5's wholesale. Vanilla countries whose land the
// conversion never touched are carried over exactly as EU5 wrote them.
int WriteUntouchedVanillaCountries(std::ostringstream& output,
    const eu5::EU5World& eu5_world,
    const eu5::VanillaCountries& vanilla_countries)
{
   std::set<std::string> converted_locations;
   for (const auto& country: eu5_world.GetCountries())
   {
      converted_locations.insert(country->GetLocations().begin(), country->GetLocations().end());
   }

   int preserved = 0;
   for (const auto& vanilla: vanilla_countries.GetCountries())
   {
      // A country the conversion took any land from has been replaced by a converted one.
      const bool overlaps = std::ranges::any_of(vanilla.locations, [&converted_locations](const auto& location) {
         return converted_locations.contains(location);
      });
      if (vanilla.locations.empty() || overlaps)
      {
         continue;
      }
      output << "\n" << vanilla.block;
      ++preserved;
   }
   return preserved;
}
}  // namespace

namespace out
{

CountriesFile::CountriesFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    const eu5::VanillaCountries& vanilla_countries):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    vanilla_countries_(vanilla_countries)
{
}

void CountriesFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   std::ostringstream output;
   output << "current_age = " << kCurrentAge << "\n\n";
   output << "countries = {\n";
   output << "\tcountries = {\n";

   for (const auto& country: eu5_world_.GetCountries())
   {
      if (ShouldWrite(*country))
      {
         WriteCountry(output, *country);
      }
   }

   const auto preserved = WriteUntouchedVanillaCountries(output, eu5_world_, vanilla_countries_);
   Log(LogLevel::Info) << "\t<> Kept " << preserved << " vanilla countries on land the conversion did not reach.";

   output << "\t}\n";
   output << "}\n";

   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
