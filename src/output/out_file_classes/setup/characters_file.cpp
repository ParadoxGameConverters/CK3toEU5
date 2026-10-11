#include "characters_file.hpp"

#include <Date.h>
#include <external/commonItems/Log.h>

#include <sstream>
#include <string>

#include "src/ck3_world/characters/character.hpp"
#include "src/ck3_world/realms/realm.hpp"
#include "src/eu5_world/eu5_country.hpp"

namespace
{
// EU5 has a single start date, START_DATE = "1337.4.1" in its defines, so a ruler born in 867
// would be five centuries old at game start.
// Birth dates are shifted by the gap between the CK3 save and this, preserving the ruler's age.
const date kGameStartDate = date("1337.4.1");

date AgeOntoStartDate(const date& birth_date, const date& conversion_date)
{
   date shifted = birth_date;
   shifted.ChangeByYears(kGameStartDate.getYear() - conversion_date.getYear());
   return shifted;
}
}  // namespace

namespace out
{

CharactersFile::CharactersFile(const std::string& name, FileWriter& file_writer, const eu5::EU5World& eu5_world):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world)
{
}

void CharactersFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   std::ostringstream output;
   output << "# Rulers converted from the CK3 save.\n\n";
   output << "character_db = {\n";

   int written = 0;
   for (const auto& country: eu5_world_.GetCountries())
   {
      if (country->GetLocations().empty() || !country->HasRuler())
      {
         continue;
      }
      const auto& holder = country->GetSourceRealm()->GetHolder();

      output << "\n\t" << country->GetRulerId() << " = { # " << country->GetRulerName() << " of "
             << country->GetSourceRealm()->GetRealmName() << "\n";
      output << "\t\tfirst_name = { name = " << country->GetRulerNameKey() << " }\n";
      output << "\t\tculture = " << *country->GetCulture() << "\n";
      output << "\t\treligion = " << *country->GetReligion() << "\n";
      output << "\t\tbirth_date = "
             << AgeOntoStartDate(holder->GetBirthDate(), eu5_world_.GetConversionDate()).toString() << "\n";
      if (country->GetCapitalLocation().has_value())
      {
         output << "\t\tbirth = " << *country->GetCapitalLocation() << "\n";
      }
      output << "\t\ttag = " << country->GetTag() << "\n";
      output << "\t}\n";
      ++written;
   }

   output << "}\n";

   Log(LogLevel::Info) << "\t<> Wrote " << written << " rulers.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
