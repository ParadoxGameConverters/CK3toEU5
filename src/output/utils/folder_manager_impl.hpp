#ifndef OUT_FOLDER_MANAGER_IMPL_H
#define OUT_FOLDER_MANAGER_IMPL_H

#include <ostream>
#include <utility>

#include "folder_manager.hpp"

namespace out
{

class FolderManagerImpl: public FolderManager
{
  public:
   void RemoveFolder(const std::filesystem::path& folder_path) override;
   void RemoveFolder(const std::string& folder_path) override;
   void CreateFolder(const std::filesystem::path& folder_path) override;
   void CreateFolder(const std::string& folder_path) override;
};

}  // namespace out

#endif  // OUT_FOLDER_MANAGER_IMPL_H