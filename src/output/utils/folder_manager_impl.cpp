#include "folder_manager_impl.hpp"

#include <external/commonItems/Log.h>
#include <external/commonItems/OSCompatibilityLayer.h>

#include <filesystem>
#include <stdexcept>
#include <string>



namespace out
{

void FolderManagerImpl::RemoveFolder(const std::filesystem::path& folder_path)
{
   if (commonItems::DoesFolderExist(folder_path))
   {
      Log(LogLevel::Info) << "Removing pre-existing copy of " << folder_path.string();
      remove_all(folder_path);
   }
}

void FolderManagerImpl::RemoveFolder(const std::string& folder_path)
{
   RemoveFolder(std::filesystem::path(folder_path));
}

void FolderManagerImpl::CreateFolder(const std::filesystem::path& folder_path)
{
   if (commonItems::DoesFolderExist(folder_path))
   {
      throw std::runtime_error("Duplicate creation of " + folder_path.string() + ". Something went very wrong.");
   }

   std::filesystem::create_directories(folder_path);
}

void FolderManagerImpl::CreateFolder(const std::string& folder_path)
{
   CreateFolder(std::filesystem::path(folder_path));
}
}  // namespace out