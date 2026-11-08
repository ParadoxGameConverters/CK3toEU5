#ifndef OUT_COUNTRY_DEFINITIONS_FILE_H
#define OUT_COUNTRY_DEFINITIONS_FILE_H

#include <filesystem>
#include <string>

#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Writes setup/countries definitions for converted tags EU5 does not define itself, so the game
// accepts them in 10_countries.txt instead of rejecting the block.
class CountryDefinitionsFile: public OutputFile
{
  public:
   CountryDefinitionsFile(const std::string& name, FileWriter& file_writer, const eu5::EU5World& eu5_world);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
};

}  // namespace out

#endif  // OUT_COUNTRY_DEFINITIONS_FILE_H
