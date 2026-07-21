#include "resource_copy.hpp"

#include <external/commonItems/Log.h>

#include <filesystem>
#include <string>
#include <utility>

#include "src/output/out_file_classes/output_file.hpp"



namespace out
{

CopyResource::CopyResource(std::string name, std::filesystem::path resource_path):
    OutputFileOrResource(std::move(name)),
    resource_path_(std::move(resource_path))
{
}

void CopyResource::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCopying resources files";
   copy("resources" / resource_path_,
       folder_path / GetName(),
       std::filesystem::copy_options::recursive);  // NOLINT - clang-tidy hates filesystem
}
}  // namespace out