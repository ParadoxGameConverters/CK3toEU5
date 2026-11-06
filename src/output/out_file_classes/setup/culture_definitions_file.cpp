#include "culture_definitions_file.hpp"

#include <external/commonItems/Log.h>

#include <cstdint>
#include <sstream>
#include <string>

namespace
{
// EU5 accepts a literal rgb colour, so a generated culture does not need a named map colour.
// Derived from the name so a culture keeps the same colour between runs.
std::string ColourFor(const std::string& name)
{
   std::uint32_t hash = 2166136261U;
   for (const char character: name)
   {
      hash ^= static_cast<std::uint8_t>(character);
      hash *= 16777619U;
   }
   const auto red = 60 + (hash >> 16U & 0xFFU) % 170;
   const auto green = 60 + (hash >> 8U & 0xFFU) % 170;
   const auto blue = 60 + (hash & 0xFFU) % 170;
   return "rgb { " + std::to_string(red) + " " + std::to_string(green) + " " + std::to_string(blue) + " }";
}
}  // namespace

namespace out
{

CultureDefinitionsFile::CultureDefinitionsFile(const std::string& name,
    FileWriter& file_writer,
    const eu5::EU5World& eu5_world):
    OutputFile(name, file_writer),
    eu5_world_(eu5_world)
{
}

void CultureDefinitionsFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating " << GetName();

   const auto generated = eu5_world_.GetCultureResolver().GetUsedGeneratedCultures();

   std::ostringstream output;
   output << "# Cultures converted from CK3 that EU5 does not define itself, including every\n";
   output << "# hybrid and divergent culture the campaign created.\n";

   for (const auto& [name, definition]: generated)
   {
      output << "\n" << name << " = {\n";
      output << "\tlanguage = " << definition.language << "\n";
      output << "\tcolor = " << ColourFor(name) << "\n";
      output << "\tculture_groups = {\n";
      for (const auto& group: definition.groups)
      {
         output << "\t\t" << group << "\n";
      }
      output << "\t}\n";
      output << "}\n";
   }

   Log(LogLevel::Info) << "\t<> Wrote " << generated.size() << " generated culture definitions.";
   UseFileWriter().CreateEmptyAndWrite(folder_path / GetName(), output.str());
}

}  // namespace out
