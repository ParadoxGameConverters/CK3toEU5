#include "folder_manager_impl.hpp"

#include <external/commonItems/Log.h>
#include <external/commonItems/OSCompatibilityLayer.h>

#include <filesystem>
#include <format>
#include <stdexcept>



namespace out
{

void FolderManagerImpl::RemoveFolder(const std::filesystem::path& folder_path)
{
   if (commonItems::DoesFolderExist(folder_path))
   {
      Log(LogLevel::Info) << std::format("Removing pre-existing copy of {}", folder_path.string());
      remove_all(folder_path);
   }
}

void FolderManagerImpl::CreateFolder(const std::filesystem::path& folder_path)
{
   if (commonItems::DoesFolderExist(folder_path))
   {
      throw std::runtime_error("Duplicate creation of " + folder_path.string() + ". Something went very wrong.");
   }

   std::filesystem::create_directories(folder_path);
}

}  // namespace out