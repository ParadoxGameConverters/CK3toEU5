#ifndef OUT_ADVISOR_FILE_H
#define OUT_ADVISOR_FILE_H

#include <external/commonItems/GameVersion.h>

#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

class AdvisorFile: public OutputFileOrResource
{
  public:
   AdvisorFile(const std::string& name, FileWriter* file_writer);

   void Create(const std::filesystem::path& /*folder_path*/) override;
};

}  // namespace out

#endif  // OUT_OUTPUT_WRITER__H