#include "output_mod.hpp"

#include <external/commonItems/GameVersion.h>

#include <filesystem>
#include <string>
#include <utility>

#include "out_file_classes/history/generic_advisors.hpp"
#include "out_file_classes/output/dot_mod_files.hpp"
#include "out_file_classes/output_folder.hpp"
#include "out_file_classes/resource_copy.hpp"



namespace out
{

OutputWriter::OutputWriter(std::string name, GameVersion eu5_version /*, EU5World eu5_world*/):
    mod_name_(std::move(name)),
    eu5_version_(std::move(eu5_version)),
    output_path_(std::filesystem::path("output"))
{
   // ----------------------------------------------------------------------------------------
   // Here all the OutputFolder and OutputFileOrResource objects representing final mod files need to be created and
   // registered according to folder structure
   // ----------------------------------------------------------------------------------------
   auto* output_folder = new OutputFolder(output_path_.string(), &folder_manager_);

   auto* dot_mod_files = new DotModFiles(mod_name_, &file_writer_, eu5_version_);
   output_folder->RegisterFileOrResource(dot_mod_files);

   auto* mod_folder = new OutputFolder(mod_name_, &folder_manager_);
   output_folder->RegisterSubfolder(mod_folder);

   // History
   // ----------------------------------
   auto* history_folder = new OutputFolder("history", &folder_manager_);
   mod_folder->RegisterSubfolder(history_folder);

   auto* generic_advisors_file = new AdvisorFile("advisors", &file_writer_ /*, EU5World eu5_world*/);
   history_folder->RegisterFileOrResource(generic_advisors_file);

   // Localisation
   // ----------------------------------
   auto* localisation_resource_folder = new CopyResource("localisation",
       std::filesystem::path("resources") / "localisation");  // makes a copy of resources/localisation
   mod_folder->RegisterFileOrResource(localisation_resource_folder);

   root_folder_ = output_folder;
}

OutputWriter::~OutputWriter()
{
   delete root_folder_;  // this deletes all folder and file objects recursively
}

void OutputWriter::GenereteOutputMod()
{
   folder_manager_.RemoveFolder(output_path_);  // Removes previous output folder

   root_folder_->CreateRecursive(
       std::filesystem::path(""));  // Creates all mod files in the newly created output folder
}


}  // namespace out