#ifndef OUT_DEVELOPMENT_FILE_H
#define OUT_DEVELOPMENT_FILE_H

#include <filesystem>
#include <string>

#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Writes setup/start development: EU5's own scoring rules, with the converted save's development
// appended as per location bonuses.
//
// The file is not a table of values. Most of it is a formula scoring each location from terrain,
// rivers, roads and settlement rank, and only the tail lists per location bonuses for notable
// places. EU5's half is therefore copied verbatim and the converted bonuses added to it.
class DevelopmentFile: public OutputFile
{
  public:
   DevelopmentFile(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       std::filesystem::path eu5_directory);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
   std::filesystem::path eu5_directory_;
};

}  // namespace out

#endif  // OUT_DEVELOPMENT_FILE_H
