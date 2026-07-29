#include "output_folder.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include "output_file.hpp"
#include "src/output/utils/folder_manager.hpp"



namespace out
{

OutputFolder::OutputFolder(std::string name, FolderManager& folder_manager):
    name_(std::move(name)),
    folder_manager_(folder_manager)
{
}

void OutputFolder::CreateRecursive(            // NOLINT - misc-no-recursion - this is purposefully recursive, recursion
    const std::filesystem::path& parent_path)  // is a standard practice when working on a folder structure
{
   const std::filesystem::path my_path = parent_path / name_;
   folder_manager_.CreateFolder(my_path);
   for (auto& ptr: files_)
   {
      ptr.get()->Create(my_path);
   }
   for (auto& ptr: subfolders_)
   {
      ptr.get()->CreateRecursive(my_path);
   }
}

void OutputFolder::RegisterSubfolder(std::unique_ptr<OutputFolder> folder)
{
   subfolders_.push_back(std::move(folder));
}

void OutputFolder::RegisterFileOrResource(std::unique_ptr<OutputFileOrResource> file)
{
   files_.push_back(std::move(file));
}

}  // namespace out