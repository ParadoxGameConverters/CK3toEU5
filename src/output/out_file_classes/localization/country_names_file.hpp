#ifndef OUT_COUNTRY_NAMES_FILE_H
#define OUT_COUNTRY_NAMES_FILE_H

#include <filesystem>
#include <string>

#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Writes the English localisation for converted countries. Without it every generated tag shows in
// game as its raw three character code.
class CountryNamesFile: public OutputFile
{
  public:
   CountryNamesFile(const std::string& name, FileWriter& file_writer, const eu5::EU5World& eu5_world);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
};

}  // namespace out

#endif  // OUT_COUNTRY_NAMES_FILE_H
