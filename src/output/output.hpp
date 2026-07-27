#ifndef OUT_OUTPUT_MOD_H
#define OUT_OUTPUT_MOD_H

#include <external/commonItems/ConverterVersion.h>

#include <ostream>
#include <utility>
#include <vector>

#include "out_file_classes/output_folder.hpp"
#include "utils/file_writer_impl.hpp"
#include "utils/folder_manager_impl.hpp"

namespace out
{

class Output  // class with the structure of the output mod
{
  public:
   Output(std::string name, commonItems::ConverterVersion& converter_version /*, EU5World eu5_world*/);

   ~Output() = default;

   Output(const Output&) = delete;
   Output& operator=(const Output&) = delete;

   Output(Output&&) noexcept = default;
   Output& operator=(Output&&) noexcept = default;

   void GenereteOutputMod();

  private:
   std::unique_ptr<OutputFolder> root_folder_;
   FolderManagerImpl folder_manager_;
   FileWriterImpl file_writer_;

   std::filesystem::path output_path_;
   std::string mod_name_;
   commonItems::ConverterVersion converter_version_;
};

}  // namespace out

#endif  // OUT_OUTPUT_MOD_H