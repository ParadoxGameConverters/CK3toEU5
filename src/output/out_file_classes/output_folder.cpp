#include "output_folder.hpp"

#include <filesystem>
#include <string>
#include <utility>

#include "output_file.hpp"
#include "src/output/utils/folder_manager.hpp"



namespace out
{

OutputFolder::OutputFolder(std::string name, FolderManager* folder_manager):
    name_(std::move(name)),
    folder_manager_(folder_manager)
{
}

OutputFolder::~OutputFolder()
{
   for (const OutputFileOrResource* file: files_)
   {
      delete file;
   }
   files_.clear();
   for (const OutputFolder* subfolder: subfolders_)
   {
      delete subfolder;
   }
   subfolders_.clear();
}

void OutputFolder::CreateRecursive(
    const std::filesystem::path& parent_path)  // NOLINT - misc-no-recursion - this is purposefully recursive, recursion
                                               // is a standard practice when working on a folder structure
{
   const std::filesystem::path my_path = parent_path / name_;
   folder_manager_->CreateFolder(my_path);
   for (OutputFileOrResource* file: files_)
   {
      file->Create(my_path);
   }
   for (OutputFolder* subfolder: subfolders_)
   {
      subfolder->CreateRecursive(my_path);
   }
}

void OutputFolder::RegisterSubfolder(OutputFolder* folder)
{
   subfolders_.push_back(folder);
}

void OutputFolder::RegisterFileOrResource(OutputFileOrResource* file)
{
   files_.push_back(file);
}

}  // namespace out