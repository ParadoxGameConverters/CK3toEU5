#ifndef OUT_OUTPUT_MOD_H
#define OUT_OUTPUT_MOD_H

#include <external/commonItems/GameVersion.h>

#include <ostream>
#include <utility>
#include <vector>

#include "out_file_classes/output_folder.hpp"
#include "utils/file_writer_impl.hpp"
#include "utils/folder_manager_impl.hpp"

namespace out
{

class OutputWriter  // class with the structure of the output mod
{
  public:
   OutputWriter(std::string name, GameVersion eu5_version);

   ~OutputWriter();

   void GenereteOutputMod();

  private:
   OutputFolder* root_folder_{nullptr};
   FolderManagerImpl folder_manager_;
   FileWriterImpl file_writer_;

   std::filesystem::path output_path_;
   std::string mod_name_;
   GameVersion eu5_version_;
};

}  // namespace out

#endif  // OUT_OUTPUT_MOD_H