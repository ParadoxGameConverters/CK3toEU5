#ifndef OUT_DIPLOMACY_FILE_H
#define OUT_DIPLOMACY_FILE_H

#include <filesystem>
#include <string>

#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Writes setup/start diplomacy: CK3's vassal relationships as EU5 subjects.
class DiplomacyFile: public OutputFile
{
  public:
   DiplomacyFile(const std::string& name, FileWriter& file_writer, const eu5::EU5World& eu5_world);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
};

}  // namespace out

#endif  // OUT_DIPLOMACY_FILE_H
