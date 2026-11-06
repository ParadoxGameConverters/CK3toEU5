#include "diplomacy_file.hpp"

#include <external/commonItems/Log.h>

#include <set>
#include <sstream>
#include <string>

#include "src/eu5_world/eu5_country.hpp"

namespace
{
// CK3 vassalage is closest to EU5's plain vassal. Other subject types carry regional baggage -
// tusi, samanta, hanseatic_member - that a converted realm has no basis for.
const std::string kSubjectType = "vassal";
}  // namespace

namespace out
{

DiplomacyFile::DiplomacyFile(const std::string& name, FileWriter& file_writer, const eu5::EU5World& eu5_world):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world)
{
}

void DiplomacyFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   // A country that ended up with no land is never written to 10_countries, so a dependency naming
   // it would point at a country EU5 does not have.
   std::set<std::string> landed_tags;
   for (const auto& country: eu5_world_.GetCountries())
   {
      if (!country->GetLocations().empty())
      {
         landed_tags.insert(country->GetTag());
      }
   }

   std::ostringstream output;
   output << "# Vassal relationships converted from the CK3 save.\n\n";
   output << "diplomacy_manager = {\n";

   int written = 0;
   for (const auto& dependency: eu5_world_.GetDependencies())
   {
      if (!landed_tags.contains(dependency.liege_tag) || !landed_tags.contains(dependency.vassal_tag))
      {
         continue;
      }
      output << "\tdependency = { first = " << dependency.liege_tag << " second = " << dependency.vassal_tag
             << " subject_type = " << kSubjectType << " }\n";
      ++written;
   }

   output << "}\n";

   Log(LogLevel::Info) << "\t<> Wrote " << written << " vassal relationships.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
