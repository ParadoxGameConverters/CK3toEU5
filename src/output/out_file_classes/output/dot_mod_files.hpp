#ifndef OUT_MOD_FILE_H
#define OUT_MOD_FILE_H

#include <external/commonItems/GameVersion.h>

#include <ostream>
#include <utility>

#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

class DotModFiles: public OutputFileOrResource
{
  public:
   DotModFiles(const std::string& name, FileWriter* file_writer, GameVersion output_eu5_version);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   GameVersion output_eu5_version_;
};

}  // namespace out

#endif  // OUT_MOD_FILE_H