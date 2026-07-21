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

   virtual void RemoveFolder(const std::filesystem::path& /*unused*/) {};
   virtual void RemoveFolder(const std::string& /*unused*/) {};
   virtual void CreateFolder(const std::filesystem::path& /*unused*/) {};
   virtual void CreateFolder(const std::string& /*unused*/) {};
};

}  // namespace out

#endif  // OUT_FOLDER_MANAGER_H