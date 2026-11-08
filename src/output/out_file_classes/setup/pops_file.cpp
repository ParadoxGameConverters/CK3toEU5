#include "pops_file.hpp"

#include <external/commonItems/Log.h>

#include <format>
#include <sstream>
#include <string>

namespace out
{

PopsFile::PopsFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    const eu5::LocationData& location_data):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    location_data_(location_data)
{
}

void PopsFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   const auto& location_religions = eu5_world_.GetLocationReligions();
   const auto& location_cultures = eu5_world_.GetLocationCultures();

   std::ostringstream output;
   output << "# Populations with religion and culture converted from the CK3 save. Pop types and\n";
   output << "# sizes are EU5's own. EU5's culture stands wherever CK3 has nothing finer to say\n";
   output << "# than the group it already belongs to; only land the two disagree about is rewritten.\n\n";
   output << "locations = {\n";

   int converted_locations = 0;
   int converted_pops = 0;
   int converted_culture_pops = 0;
   for (const auto& [location, pops]: location_data_.GetPops())
   {
      output << "\n\t" << location << " = {\n";

      const auto converted_religion = location_religions.find(location);
      const bool convert = converted_religion != location_religions.end();
      if (convert)
      {
         ++converted_locations;
      }
      const auto converted_culture = location_cultures.find(location);
      const bool convert_culture = converted_culture != location_cultures.end();

      // Only the majority faith and culture are converted. EU5's own minorities - the Jewish
      // burghers of Paris, for instance - have no counterpart in CK3's single faith and culture
      // per county, and overwriting them would quietly erase them from the map.
      const auto vanilla_majority = location_data_.GetDominantReligion(location);
      const auto vanilla_culture_majority = location_data_.GetDominantCulture(location);

      for (const auto& pop: pops)
      {
         const bool convert_this_pop = convert && pop.religion == vanilla_majority;
         const auto& religion = convert_this_pop ? converted_religion->second : pop.religion;
         if (convert_this_pop && religion != pop.religion)
         {
            ++converted_pops;
         }

         const bool convert_this_culture = convert_culture && pop.culture == vanilla_culture_majority;
         const auto& culture = convert_this_culture ? converted_culture->second : pop.culture;
         if (convert_this_culture && culture != pop.culture)
         {
            ++converted_culture_pops;
         }

         output << std::format("\t\tdefine_pop = {{ type = {} size = {:.3f} culture = {} religion = {} }}\n",
             pop.type,
             pop.size,
             culture,
             religion);
      }
      output << "\t}\n";
   }

   output << "}\n";

   Log(LogLevel::Info) << "\t<> Wrote pops for " << location_data_.GetPops().size() << " locations, converting "
                       << converted_pops << " pops across " << converted_locations << " locations, and "
                       << converted_culture_pops << " pop cultures across " << location_cultures.size()
                       << " locations.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
