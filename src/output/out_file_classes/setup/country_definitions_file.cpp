#include "country_definitions_file.hpp"

#include <external/commonItems/Log.h>

#include <cstdint>
#include <sstream>
#include <string>

#include "src/ck3_world/realms/realm.hpp"
#include "src/ck3_world/titles/title.hpp"
#include "src/eu5_world/eu5_country.hpp"

namespace
{
// EU5 accepts a literal rgb colour, so a converted country does not need a named map colour.
// Derived from the tag so a given realm keeps the same colour between runs.
std::string ColourFor(const std::string& tag)
{
   std::uint32_t hash = 2166136261U;
   for (const char character: tag)
   {
      hash ^= static_cast<std::uint8_t>(character);
      hash *= 16777619U;
   }
   // Kept mid range so countries stay legible against the map.
   const auto red = 60 + (hash >> 16U & 0xFFU) % 170;
   const auto green = 60 + (hash >> 8U & 0xFFU) % 170;
   const auto blue = 60 + (hash & 0xFFU) % 170;
   return "rgb { " + std::to_string(red) + " " + std::to_string(green) + " " + std::to_string(blue) + " }";
}
}  // namespace

namespace out
{

CountryDefinitionsFile::CountryDefinitionsFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world)
{
}

void CountryDefinitionsFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   std::ostringstream output;
   output << "# Country definitions for converted tags EU5 does not ship.\n";

   int written = 0;
   for (const auto& country: eu5_world_.GetCountries())
   {
      if (!country->NeedsDefinition() || country->GetLocations().empty())
      {
         continue;
      }
      // Without a culture and a religion EU5 will not accept the definition, so skip rather than
      // write something it will reject.
      if (!country->GetCulture().has_value() || !country->GetReligion().has_value())
      {
         continue;
      }

      output << "\n"
             << country->GetTag() << " = { # " << country->GetSourceRealm()->GetRealmName() << " ("
             << country->GetSourceRealm()->GetPrimaryTitle()->GetKey() << ")\n";
      output << "\tcolor = " << ColourFor(country->GetTag()) << "\n";
      output << "\tculture_definition = " << *country->GetCulture() << "\n";
      output << "\treligion_definition = " << *country->GetReligion() << "\n";
      output << "}\n";
      ++written;
   }

   Log(LogLevel::Info) << "\t<> Wrote " << written << " generated country definitions.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
