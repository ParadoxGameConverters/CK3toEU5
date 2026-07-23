#include "dot_mod_files.hpp"

#include <external/commonItems/GameVersion.h>
#include <external/commonItems/Log.h>

#include <filesystem>
#include <format>
#include <string>
#include <utility>

#include "src/output/out_file_classes/output_file.hpp"
#include "src/output/utils/file_writer.hpp"



namespace out
{

DotModFiles::DotModFiles(const std::string& name, FileWriter* file_writer, GameVersion output_eu5_version):
    OutputFileOrResource(name, file_writer),
    output_eu5_version_(std::move(output_eu5_version))
{
}

void DotModFiles::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating .mod files";

   UseFileWriter()->CreateEmptyAndWrite(folder_path / std::format("{}.mod", GetName()),
       std::format("name = \"Converted - {}\"\n"
                   "path = \"mod/{}/\"\n"
                   "user_dir = \"{}_user_dir\"\n"
                   "replace_path=\"common/countries\"\n"
                   "replace_path=\"common/national_focus\"\n"
                   "replace_path=\"common/peace_conference/ai_peace\"\n"
                   "replace_path=\"common/peace_conference/cost_modifiers\"\n"
                   "replace_path=\"events\"\n"
                   "replace_path=\"history/countries\"\n"
                   "replace_path=\"history/states\"\n"
                   "replace_path=\"history/units\"\n"
                   "replace_path=\"map/supplyareas\"\n"
                   "replace_path=\"map/strategicregions\"\n"
                   "supported_version=\"{}\"",
           GetName(),
           GetName(),
           GetName(),
           output_eu5_version_.toWildCard()));

   UseFileWriter()->CreateEmptyAndWrite(folder_path / std::format("descriptor.mod", GetName()),
       std::format("name = \"Converted - {}\"\n"
                   "replace_path=\"common/countries\"\n"
                   "replace_path=\"common/national_focus\"\n"
                   "replace_path=\"common/peace_conference/ai_peace\"\n"
                   "replace_path=\"common/peace_conference/cost_modifiers\"\n"
                   "replace_path=\"events\"\n"
                   "replace_path=\"history/countries\"\n"
                   "replace_path=\"history/states\"\n"
                   "replace_path=\"history/units\"\n"
                   "replace_path=\"map/supplyareas\"\n"
                   "replace_path=\"map/strategicregions\"\n"
                   "supported_version=\"{}\"",
           GetName(),
           output_eu5_version_.toWildCard()));
}

}  // namespace out