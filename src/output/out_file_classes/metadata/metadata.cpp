#include "metadata.hpp"

#include <external/commonItems/ConverterVersion.h>
#include <external/commonItems/GameVersion.h>
#include <external/commonItems/Log.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <string>
#include <utility>

#include "src/output/out_file_classes/output_file.hpp"
#include "src/output/utils/file_writer.hpp"



namespace out
{

MetadataFile::MetadataFile(const std::string& name,
    FileWriter& file_writer,
    commonItems::ConverterVersion converter_version):
    OutputFile(name, file_writer),
    converter_version_(std::move(converter_version))
{
}

void MetadataFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating .mod files";

   std::string mod_id = GetName();
   mod_id.erase(std::remove(mod_id.begin(), mod_id.end(), ' '), mod_id.end());

   UseFileWriter().CreateEmptyAndWrite(folder_path / "metadata.json",
       std::format("{{\n"
                   "\t\"name\": \"{}\",\n"
                   "\t\"id\": \"{}\",\n"
                   "\t\"version\": \"{}\",\n"
                   "\t\"supported_game_version\": \"{}\",\n"
                   "\t\"short_description\": \"Mod converting CK3 to EU5\",\n"
                   "\t\"tags\": [\"Alternative History\", \"Overhaul\"],\n"
                   "\t\"relationships\": [],\n"
                   "\t\"game_custom_data\": {{\n"
                   "\t\t\"replace_paths\": [\n"
                   "\t\t]\n"
                   "\t}}\n"
                   "}}",
           GetName(),
           mod_id,
           converter_version_.getVersion(),
           converter_version_.getMaxTarget().toWildCard()));
}

}  // namespace out