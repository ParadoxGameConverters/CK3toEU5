#include "generic_advisors.hpp"

#include <external/commonItems/Log.h>

#include <filesystem>
#include <string>

#include "src/output/out_file_classes/output_file.hpp"
#include "src/output/utils/file_writer.hpp"



namespace out
{

AdvisorFile::AdvisorFile(const std::string& name, FileWriter& file_writer): OutputFile(name, file_writer)
{
}

void AdvisorFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating advisors file " + folder_path.string();

   auto var = std::string("zaba 123 321");

   UseFileWriter().CreateEmptyAndWrite(folder_path / "advisors.txt", var);
}

}  // namespace out