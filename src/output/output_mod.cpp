#include "output_mod.hpp"

#include <external/commonItems/GameVersion.h>

#include <string>
#include <utility>

#include "out_file_classes/history/generic_advisors.hpp"
#include "out_file_classes/output_folder.hpp"
#include "out_file_classes/resource_copy.hpp"
#include "out_file_classes/root/dot_mod_files.hpp"



namespace out
{

OutputWriter::OutputWriter(std::string name, GameVersion eu5_version /*, EU5World eu5_world*/):
    mod_name_(std::move(name)),
    eu5_version_(std::move(eu5_version))
{
   // OutputFolder* root_folder = BuildFolderStructure(name, eu5_version, folder_manager /*, EU5World eu5_world*/);
   output_path_ = std::filesystem::path("output");
}

OutputWriter::~OutputWriter()
{
   delete root_folder_;  // this deletes all folder and file objects recursively
}

void OutputWriter::BuildFolderStructure(
    /*EU5World eu5_world*/)
{
   // ----------------------------------------------------------------------------------------
   // Here all the OutputFolder and OutputFileOrResource objects representing final mod files need to be created and
   // registered according to folder structure
   // ----------------------------------------------------------------------------------------
   auto* root_folder = new OutputFolder(mod_name_, &folder_manager_);

   auto* dot_mod_files = new DotModFiles(mod_name_, &file_writer_, eu5_version_);
   root_folder->RegisterFileOrResource(dot_mod_files);

   auto* history_folder = new OutputFolder("history", &folder_manager_);
   root_folder->RegisterSubfolder(history_folder);

   auto* generic_advisors_file = new AdvisorFile("advisors", &file_writer_ /*, EU5World eu5_world*/);
   history_folder->RegisterFileOrResource(generic_advisors_file);

   auto* localisation_resource_folder = new CopyResource("localisation",
       std::filesystem::path("localisation"));  // makes a copy of resources/localisation
   root_folder->RegisterFileOrResource(localisation_resource_folder);

   root_folder_ = root_folder;
}

void OutputWriter::GenereteOutputMod()
{
   folder_manager_.RemoveFolder(output_path_);  // Prepares output folder
   folder_manager_.CreateFolder(output_path_);
   root_folder_->CreateRecursive(output_path_);  // Creates all mod files in the newly created output folder
}


}  // namespace out