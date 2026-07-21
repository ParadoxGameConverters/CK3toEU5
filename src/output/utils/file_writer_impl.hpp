#ifndef OUT_FILE_WRITER_IMPL_H
#define OUT_FILE_WRITER_IMPL_H

#include <external/commonItems/GameVersion.h>

#include <ostream>
#include <utility>

#include "file_writer.hpp"

namespace out
{

class FileWriterImpl: public FileWriter
{
  public:
   FileWriterImpl() = default;

   void CreateEmptyAndWrite(const std::filesystem::path& file_path, const std::string& content) override;
};

}  // namespace out

#endif  // OUT_FILE_WRITER_IMPL_H