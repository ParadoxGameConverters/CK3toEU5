#include "country_names_file.hpp"

#include <external/commonItems/Log.h>

#include <set>
#include <sstream>
#include <string>

#include "src/ck3_world/realms/realm.hpp"
#include "src/ck3_world/titles/title.hpp"
#include "src/eu5_world/eu5_country.hpp"

namespace
{
// Paradox localisation files are UTF-8 with a byte order mark; the game will not read them without.
const std::string kByteOrderMark = "\xEF\xBB\xBF";

// CK3 names arrive with control characters around dynamic name segments, and a stray quote would
// end the localisation string early.
std::string Sanitize(const std::string& name)
{
   std::string clean;
   for (const char character: name)
   {
      if (static_cast<unsigned char>(character) < 0x20)
      {
         continue;
      }
      if (character == '"')
      {
         clean += '\'';
         continue;
      }
      clean += character;
   }
   return clean;
}
}  // namespace

namespace out
{

CountryNamesFile::CountryNamesFile(const std::string& name, FileWriter& file_writer, const eu5::EU5World& eu5_world):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world)
{
}

void CountryNamesFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   std::ostringstream output;
   output << kByteOrderMark << "l_english:\n";

   int written = 0;
   std::set<std::string> ruler_names;
   for (const auto& country: eu5_world_.GetCountries())
   {
      if (country->GetLocations().empty())
      {
         continue;
      }
      const auto name = Sanitize(eu5::CleanCK3Name(country->GetSourceRealm()->GetRealmName()));
      if (name.empty())
      {
         continue;
      }
      output << " " << country->GetTag() << ": \"" << name << "\"\n";
      ++written;

      // Ruler names are written as keys so the game shows them properly rather than a raw token.
      if (country->HasRuler())
      {
         const auto key = country->GetRulerNameKey();
         if (!key.empty() && ruler_names.insert(key).second)
         {
            output << " " << key << ": \"" << Sanitize(country->GetRulerName()) << "\"\n";
         }
      }
   }

   Log(LogLevel::Info) << "\t<> Wrote " << written << " country names.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
