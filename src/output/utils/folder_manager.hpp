#ifndef OUT_FOLDER_MANAGER_H
#define OUT_FOLDER_MANAGER_H

#include <external/commonItems/GameVersion.h>

#include <ostream>
#include <utility>

namespace out
{

class FolderManager
{
  public:
   // FolderManager() {};
   virtual ~FolderManager() = default;

   // LCOV_EXCL_START - by default coverage expexcts us to test these "methods"
   virtual void RemoveFolder(const std::filesystem::path& /*unused*/) {};
   virtual void RemoveFolder(const std::string& /*unused*/) {};
   virtual void CreateFolder(const std::filesystem::path& /*unused*/) {};
   virtual void CreateFolder(const std::string& /*unused*/) {};
   // LCOV_EXCL_STOP
};

}  // namespace out

#endif  // OUT_FOLDER_MANAGER_H