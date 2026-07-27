#include "output.hpp"

#include <external/commonItems/ConverterVersion.h>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include "out_file_classes/history/generic_advisors.hpp"
#include "out_file_classes/metadata/metadata.hpp"
#include "out_file_classes/output_folder.hpp"
#include "out_file_classes/resource_copy.hpp"



namespace out
{

Output::Output(std::string name, commonItems::ConverterVersion& converter_version /*, EU5World eu5_world*/):
    mod_name_(std::move(name)),
    converter_version_(std::move(converter_version)),
    output_path_(std::filesystem::path("output"))
{
   // ----------------------------------------------------------------------------------------
   // Here all the OutputFolder and OutputFileOrResource objects representing final mod files need to be created and
   // registered according to folder structure
   // ----------------------------------------------------------------------------------------
   root_folder_ = std::make_unique<OutputFolder>(output_path_.string(), folder_manager_);

   auto mod_folder = std::make_unique<OutputFolder>(mod_name_, folder_manager_);

   // Metadata
   auto metadata_folder = std::make_unique<OutputFolder>(".metadata", folder_manager_);

   auto metadata_file = std::make_unique<MetadataFile>(mod_name_, file_writer_, converter_version_);

   metadata_folder->RegisterFileOrResource(std::move(metadata_file));

   mod_folder->RegisterSubfolder(std::move(metadata_folder));

   // History
   // ----------------------------------
   auto history_folder = std::make_unique<OutputFolder>("history", folder_manager_);

   auto generic_advisors_file = std::make_unique<AdvisorFile>("advisors", file_writer_ /*, EU5World eu5_world*/);

   history_folder->RegisterFileOrResource(std::move(generic_advisors_file));

   mod_folder->RegisterSubfolder(std::move(history_folder));

   // Localisation
   // ----------------------------------
   auto localisation_resource_folder = std::make_unique<CopyResource>("localisation",
       std::filesystem::path("resources") / "localisation");  // makes a copy of resources/localisation
   mod_folder->RegisterFileOrResource(std::move(localisation_resource_folder));


   // ----------------------------------
   root_folder_->RegisterSubfolder(std::move(mod_folder));
}

void Output::GenereteOutputMod()
{
   folder_manager_.RemoveFolder(output_path_);  // Removes previous output folder

   root_folder_->CreateRecursive(
       std::filesystem::path(""));  // Creates all mod files in the newly created output folder
}


}  // namespace out