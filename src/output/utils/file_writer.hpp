#ifndef OUT_FILE_WRITER_H
#define OUT_FILE_WRITER_H

#include <external/commonItems/GameVersion.h>

#include <ostream>
#include <utility>

namespace out
{

class FileWriter
{
  public:
   // FileWriter() {};
   virtual ~FileWriter() = default;

   virtual void CreateEmptyAndWrite(const std::filesystem::path& /*unused*/, const std::string& /*unused*/) = 0;
};

}  // namespace out

#endif  // OUT_FILE_WRITER__H