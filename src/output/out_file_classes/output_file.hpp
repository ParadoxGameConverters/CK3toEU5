#ifndef OUT_OUTPUT_FILE_H
#define OUT_OUTPUT_FILE_H

#include <external/commonItems/GameVersion.h>

#include <ostream>
#include <utility>

#include "src/output/utils/file_writer.hpp"
#include "src/output/utils/folder_manager.hpp"

namespace out
{

class OutputFileOrResource
{
  public:
   explicit OutputFileOrResource(std::string name): name_(std::move(name)) {}
   virtual ~OutputFileOrResource() = default;
   virtual void Create(const std::filesystem::path& /*path*/) = 0;

   OutputFileOrResource(const OutputFileOrResource&) = delete;
   OutputFileOrResource& operator=(const OutputFileOrResource&) = delete;

   OutputFileOrResource(OutputFileOrResource&&) noexcept = delete;
   OutputFileOrResource& operator=(OutputFileOrResource&&) noexcept = delete;

  protected:
   std::string GetName() { return name_; }

  private:
   std::string name_;
};

class OutputFile: public OutputFileOrResource
{
  public:
   OutputFile(std::string name, FileWriter& file_writer):
       OutputFileOrResource(std::move(name)),
       file_writer_(file_writer)
   {
   }  // here it will take EU5 world as well

  protected:
   FileWriter& UseFileWriter() { return file_writer_; }

  private:
   FileWriter& file_writer_;
};

}  // namespace out

#endif  // OUT_OUTPUT_FILE_H