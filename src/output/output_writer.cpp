#include "output_writer.hpp"

#include <external/commonItems/GameVersion.h>
#include <external/commonItems/Log.h>
#include <external/commonItems/OSCompatibilityLayer.h>

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>



namespace out
{

OutputWriter::OutputWriter(const std::string& output_name, GameVersion output_eu5_version):
    output_eu5_version_(std::move(output_eu5_version))
{
   output_name_path_ = std::filesystem::path(output_name);
   ;
}

void OutputWriter::PrepareOutputFolder()
{
   Log(LogLevel::Info) << "Preparing output folder";
   ClearOutputFolder();
   CreateOutputFolder();
}

void OutputWriter::OutputMod()
{
   Log(LogLevel::Info) << "Outputting mod";
   CreateModFiles();
}

void OutputWriter::ClearOutputFolder()
{
   const std::filesystem::path output_folder = "output" / output_name_path_;
   if (commonItems::DoesFolderExist(output_folder))
   {
      Log(LogLevel::Info) << "Removing pre-existing copy of " << output_name_path_.string();
      if (remove_all(output_folder) == static_cast<std::uintmax_t>(-1))
      {
         throw std::runtime_error("Could not remove pre-existing output folder " + output_folder.string() +
                                  ". Please delete folder and try converting again.");
      }
   }
}


void OutputWriter::CreateOutputFolder()
{
   CopyBlankMod();

   CreateSubfolder("common");
   CreateSubfolder("common/countries");
   CreateSubfolder("common/country_tags");
   CreateSubfolder("history");
   CreateSubfolder("history/countries");
   CreateSubfolder("history/states");
   CreateSubfolder("history/units");
   CreateSubfolder("map");
   CreateSubfolder("map/strategicregions");
}

void OutputWriter::CopyBlankMod()
{
   Log(LogLevel::Info) << "\tCopying blank mod";
   if (!commonItems::DoesFolderExist("output"))
   {
      if (!std::filesystem::create_directories("output"))
      {
         throw std::runtime_error("Could not create output folder");
      }
   }
   try
   {
      copy("blank_mod", "output" / output_name_path_, std::filesystem::copy_options::recursive);
   }
   catch (...)
   {
      throw std::runtime_error("Could not copy blank_mod");
   }
}

void OutputWriter::CreateSubfolder(std::string_view subfolder)
{
   if (const std::filesystem::path path = "output" / output_name_path_ / subfolder;
       !commonItems::DoesFolderExist(path) && !std::filesystem::create_directories(path))
   {
      throw std::runtime_error(std::format("Could not create {}", path.string()));
   }
}

void OutputWriter::CreateModFiles()
{
   Log(LogLevel::Info) << "\tCreating .mod files";

   const auto output_name = output_name_path_.string();
   CreateModFile(output_name);
   CreateDescriptorFile(output_name);
}

void OutputWriter::CreateModFile(const std::string& output_name)
{
   std::ofstream mod_file(std::format("output/{}.mod", output_name));
   if (!mod_file.is_open())
   {
      throw std::runtime_error("Could not create .mod file");
   }
   std::print(mod_file,
       "name = \"Converted - {}\"\n"
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
       output_name,
       output_name,
       output_name,
       output_eu5_version_.toWildCard());
   mod_file.close();
}

void OutputWriter::CreateDescriptorFile(const std::string& output_name)
{
   std::ofstream descriptor_file(std::format("output/{}/descriptor.mod", output_name));
   if (!descriptor_file.is_open())
   {
      throw std::runtime_error("Could not create descriptor.mod");
   }
   std::print(descriptor_file,
       "name = \"Converted - {}\"\n"
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
       output_name,
       output_eu5_version_.toWildCard());
   descriptor_file.close();
}

}  // namespace out