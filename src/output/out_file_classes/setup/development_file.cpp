#include "development_file.hpp"

#include <external/commonItems/Log.h>

#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace
{
const std::filesystem::path kVanillaFile =
    std::filesystem::path("game") / "main_menu" / "setup" / "start" / "14_development.txt";
}  // namespace

namespace out
{

DevelopmentFile::DevelopmentFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world,
    std::filesystem::path eu5_directory):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world),
    eu5_directory_(std::move(eu5_directory))
{
}

void DevelopmentFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   std::ifstream vanilla(eu5_directory_ / kVanillaFile);
   if (!vanilla.is_open())
   {
      Log(LogLevel::Warning) << "\t<> EU5 development rules not found - skipping, vanilla will apply.";
      return;
   }

   // Everything up to the final closing brace is EU5's scoring formula and its own location
   // bonuses, which the converter has no business rewriting.
   std::string contents((std::istreambuf_iterator<char>(vanilla)), std::istreambuf_iterator<char>());
   const auto last_brace = contents.find_last_of('}');
   if (last_brace == std::string::npos)
   {
      Log(LogLevel::Warning) << "\t<> EU5 development rules look malformed - skipping.";
      return;
   }

   std::ostringstream output;
   output << contents.substr(0, last_brace);
   output << "\n\t# Development the CK3 save built beyond its own median, so a player's work on\n";
   output << "\t# their realm carries over instead of every location falling back to terrain.\n";
   for (const auto& [location, bonus]: eu5_world_.GetDevelopmentBonuses())
   {
      output << "\t" << location << " = " << bonus << "\n";
   }
   output << "}\n";

   Log(LogLevel::Info) << "\t<> Wrote " << eu5_world_.GetDevelopmentBonuses().size()
                       << " converted location development bonuses.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
