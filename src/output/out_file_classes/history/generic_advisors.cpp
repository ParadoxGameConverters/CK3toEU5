#include "generic_advisors.hpp"

#include <external/commonItems/Log.h>

#include <filesystem>
#include <string>

#include "src/output/out_file_classes/output_file.hpp"
#include "src/output/utils/file_writer.hpp"



namespace out
{

AdvisorFile::AdvisorFile(const std::string& name, FileWriter* file_writer): OutputFileOrResource(name, file_writer)
{
}

void AdvisorFile::Create(const std::filesystem::path& folder_path)
{
   Log(LogLevel::Info) << "\tCreating advisors file " + folder_path.string();

   auto var = std::string("zaba 123 321");

   UseFileWriter()->CreateEmptyAndWrite(folder_path / "advisors.txt", var);
}

// void RootFolder::CreateModFile()
//{
//
// }
//
// void RootFolder::CreateDescriptorFile()
//{
//    std::ofstream descriptor_file(std::format("output/{}/descriptor.mod", output_name_));
//    if (!descriptor_file.is_open())
//    {
//       throw std::runtime_error("Could not create descriptor.mod");
//    }
//    std::print(descriptor_file,
//        "name = \"Converted - {}\"\n"
//        "replace_path=\"common/countries\"\n"
//        "replace_path=\"common/national_focus\"\n"
//        "replace_path=\"common/peace_conference/ai_peace\"\n"
//        "replace_path=\"common/peace_conference/cost_modifiers\"\n"
//        "replace_path=\"events\"\n"
//        "replace_path=\"history/countries\"\n"
//        "replace_path=\"history/states\"\n"
//        "replace_path=\"history/units\"\n"
//        "replace_path=\"map/supplyareas\"\n"
//        "replace_path=\"map/strategicregions\"\n"
//        "supported_version=\"{}\"",
//        output_name_,
//        output_eu5_version_.toWildCard());
//    descriptor_file.close();
// }

}  // namespace out