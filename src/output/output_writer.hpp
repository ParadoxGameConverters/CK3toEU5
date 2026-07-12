#ifndef OUT_OUTPUT_WRITER_H
#define OUT_OUTPUT_WRITER_H

#include <external/commonItems/GameVersion.h>

#include <ostream>
#include <utility>

namespace out
{

class OutputWriter
{
  public:
   OutputWriter(const std::string& output_name, GameVersion output_eu5_version);
   void OutputMod();
   void PrepareOutputFolder();

  private:
   std::filesystem::path output_name_path_;
   GameVersion output_eu5_version_;

   void ClearOutputFolder();
   void CreateOutputFolder();
   void CreateSubfolder(std::string_view subfolder);
   void CopyBlankMod();
   void CreateModFiles();
   void CreateModFile(const std::string& output_name);
   void CreateDescriptorFile(const std::string& output_name);
};

}  // namespace out

#endif  // OUT_HOI4_OUT_MOD_H