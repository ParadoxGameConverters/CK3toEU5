#include "output.hpp"

#include <external/commonItems/ConverterVersion.h>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include "out_file_classes/localization/country_names_file.hpp"
#include "out_file_classes/metadata/metadata.hpp"
#include "out_file_classes/output_folder.hpp"
#include "out_file_classes/setup/characters_file.hpp"
#include "out_file_classes/setup/countries_file.hpp"
#include "out_file_classes/setup/country_definitions_file.hpp"
#include "out_file_classes/setup/culture_definitions_file.hpp"
#include "out_file_classes/setup/development_file.hpp"
#include "out_file_classes/setup/diplomacy_file.hpp"
#include "out_file_classes/setup/pops_file.hpp"



namespace out
{

Output::Output(std::string name,
    commonItems::ConverterVersion& converter_version,
    const eu5::EU5World& eu5_world,
    const eu5::LocationData& location_data,
    const eu5::VanillaCountries& vanilla_countries,
    const std::filesystem::path& eu5_directory):
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

   // Setup
   // ----------------------------------
   // The mod mirrors the game's own folder layout, so the start files live under
   // main_menu/setup/start, matching game/main_menu/setup/start in the EU5 install.
   auto main_menu_folder = std::make_unique<OutputFolder>("main_menu", folder_manager_);
   auto setup_folder = std::make_unique<OutputFolder>("setup", folder_manager_);
   auto start_folder = std::make_unique<OutputFolder>("start", folder_manager_);

   auto countries_file =
       std::make_unique<CountriesFile>("10_countries.txt", file_writer_, eu5_world, vanilla_countries);
   start_folder->RegisterFileOrResource(std::move(countries_file));

   auto characters_file = std::make_unique<CharactersFile>("05_characters.txt", file_writer_, eu5_world);
   start_folder->RegisterFileOrResource(std::move(characters_file));

   auto pops_file = std::make_unique<PopsFile>("06_pops.txt", file_writer_, eu5_world, location_data);
   start_folder->RegisterFileOrResource(std::move(pops_file));

   auto diplomacy_file = std::make_unique<DiplomacyFile>("12_diplomacy.txt", file_writer_, eu5_world);
   start_folder->RegisterFileOrResource(std::move(diplomacy_file));

   auto development_file =
       std::make_unique<DevelopmentFile>("14_development.txt", file_writer_, eu5_world, eu5_directory);
   start_folder->RegisterFileOrResource(std::move(development_file));

   setup_folder->RegisterSubfolder(std::move(start_folder));
   main_menu_folder->RegisterSubfolder(std::move(setup_folder));

   // Localisation sits under main_menu too, matching the game and its DLC.
   auto localization_folder = std::make_unique<OutputFolder>("localization", folder_manager_);
   auto english_folder = std::make_unique<OutputFolder>("english", folder_manager_);

   auto country_names_file =
       std::make_unique<CountryNamesFile>("00_converted_countries_l_english.yml", file_writer_, eu5_world);
   english_folder->RegisterFileOrResource(std::move(country_names_file));

   localization_folder->RegisterSubfolder(std::move(english_folder));
   main_menu_folder->RegisterSubfolder(std::move(localization_folder));

   mod_folder->RegisterSubfolder(std::move(main_menu_folder));

   // Country definitions live under in_game rather than main_menu, mirroring the game again.
   auto in_game_folder = std::make_unique<OutputFolder>("in_game", folder_manager_);
   auto in_game_setup_folder = std::make_unique<OutputFolder>("setup", folder_manager_);
   auto countries_folder = std::make_unique<OutputFolder>("countries", folder_manager_);

   auto country_definitions_file =
       std::make_unique<CountryDefinitionsFile>("00_converted_countries.txt", file_writer_, eu5_world);
   countries_folder->RegisterFileOrResource(std::move(country_definitions_file));

   in_game_setup_folder->RegisterSubfolder(std::move(countries_folder));
   in_game_folder->RegisterSubfolder(std::move(in_game_setup_folder));

   // Generated cultures live under in_game/common/cultures, mirroring the game again.
   auto in_game_common_folder = std::make_unique<OutputFolder>("common", folder_manager_);
   auto cultures_folder = std::make_unique<OutputFolder>("cultures", folder_manager_);

   auto culture_definitions_file =
       std::make_unique<CultureDefinitionsFile>("00_converted_cultures.txt", file_writer_, eu5_world);
   cultures_folder->RegisterFileOrResource(std::move(culture_definitions_file));

   in_game_common_folder->RegisterSubfolder(std::move(cultures_folder));
   in_game_folder->RegisterSubfolder(std::move(in_game_common_folder));
   mod_folder->RegisterSubfolder(std::move(in_game_folder));

   // The scaffold also registered a history/advisors.txt holding the placeholder "zaba 123 321" and
   // copied resources/localisation, whose only content is a 3 byte stub. Both shipped inside the
   // generated mod. AdvisorFile and CopyResource are kept and still tested; they just need real
   // content before they go back into the output.

   // ----------------------------------
   root_folder_->RegisterSubfolder(std::move(mod_folder));
}

void Output::GenerateOutputMod()
{
   folder_manager_.RemoveFolder(output_path_);  // Removes previous output folder

   root_folder_->CreateRecursive(
       std::filesystem::path(""));  // Creates all mod files in the newly created output folder
}


}  // namespace out