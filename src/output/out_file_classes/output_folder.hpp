#ifndef OUT_OUTPUT_FOLDER_H
#define OUT_OUTPUT_FOLDER_H

#include <external/commonItems/GameVersion.h>

#include <ostream>
#include <utility>

#include "output_file.hpp"
#include "src/output/utils/folder_manager.hpp"

namespace out
{

class OutputFolder
{
  public:
   OutputFolder(std::string name, FolderManager* folder_manager);
   ~OutputFolder();
   void CreateRecursive(const std::filesystem::path& parent_path);

   void RegisterSubfolder(OutputFolder* folder);
   void RegisterFileOrResource(OutputFileOrResource* file);

  private:
   std::vector<OutputFolder*> subfolders_;
   std::vector<OutputFileOrResource*> files_;
   std::string name_;
   FolderManager* folder_manager_;
};

}  // namespace out

#endif  // OUT_OUTPUT_WRITER__H