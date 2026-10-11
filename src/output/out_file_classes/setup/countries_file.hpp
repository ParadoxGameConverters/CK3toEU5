#ifndef OUT_COUNTRIES_FILE_H
#define OUT_COUNTRIES_FILE_H

#include <filesystem>
#include <string>

#include "src/eu5_world/eu5_vanilla_countries.hpp"
#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Writes setup/start/10_countries.txt: which EU5 locations each country owns at the start date.
class CountriesFile: public OutputFile
{
  public:
   CountriesFile(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       const eu5::VanillaCountries& vanilla_countries);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
   const eu5::VanillaCountries& vanilla_countries_;
};

}  // namespace out

#endif  // OUT_COUNTRIES_FILE_H
