#ifndef OUT_MOD_FILE_H
#define OUT_MOD_FILE_H

#include <external/commonItems/ConverterVersion.h>
#include <external/commonItems/GameVersion.h>

#include <ostream>
#include <utility>

#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

class MetadataFile: public OutputFile
{
  public:
   MetadataFile(const std::string& name, FileWriter& file_writer, commonItems::ConverterVersion converter_version);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   commonItems::ConverterVersion converter_version_;
};

}  // namespace out

#endif  // OUT_MOD_FILE_H