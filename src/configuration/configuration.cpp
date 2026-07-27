#include "configuration.hpp"

#include <external/commonItems/ConverterVersion.h>
#include <external/commonItems/GameVersion.h>
#include <external/commonItems/Log.h>
#include <external/commonItems/OSCompatibilityLayer.h>
#include <external/fmt/include/fmt/format.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>



namespace configuration
{

std::filesystem::path Configuration::GetCK3Directory() const
{
   return ck3_directory_;
}
std::filesystem::path Configuration::GetCK3DocDirectory() const
{
   return ck3_doc_directory_;
}
std::filesystem::path Configuration::GetEU5Directory() const
{
   return eu5_directory_;
}
std::filesystem::path Configuration::GetEU5ModPath() const
{
   return eu5_mod_path_;
}
std::filesystem::path Configuration::GetSaveGamePath() const
{
   return save_game_;
}
bool Configuration::GetDebug() const
{
   return debug_;
}
std::string Configuration::GetOutputName() const
{
   return output_name_;
}

void Configuration::SetCK3Directory(std::filesystem::path ck3_dir)
{
   ck3_directory_ = std::move(ck3_dir);
}
void Configuration::SetCK3DocDirectory(std::filesystem::path ck3_doc_dir)
{
   ck3_doc_directory_ = std::move(ck3_doc_dir);
}
void Configuration::SetEU5Directory(std::filesystem::path eu5_dir)
{
   eu5_directory_ = std::move(eu5_dir);
}
void Configuration::SetEU5ModPath(std::filesystem::path eu5_mod_p)
{
   eu5_mod_path_ = std::move(eu5_mod_p);
}
void Configuration::SetSaveGamePath(std::filesystem::path save_game_p)
{
   save_game_ = std::move(save_game_p);
}
void Configuration::SetDebug(bool debug)
{
   debug_ = debug;
}
void Configuration::SetOutputName(const std::string& name)
{
   output_name_ = name;
}
bool Configuration::GetShatterEmpires() const
{
   return shatter_empires_;
}
bool Configuration::GetVassalSplitoff() const
{
   return vassal_splitoff_;
}
std::string Configuration::GetHREMode() const
{
   return hre_mode_;
}
bool Configuration::GetDevImport() const
{
   return dev_import_;
}
void Configuration::SetShatterEmpires(bool shatter)
{
   shatter_empires_ = shatter;
}
void Configuration::SetVassalSplitoff(bool splitoff)
{
   vassal_splitoff_ = splitoff;
}
void Configuration::SetHREMode(const std::string& mode)
{
   hre_mode_ = mode;
}
void Configuration::SetDevImport(bool dev_import)
{
   dev_import_ = dev_import;
}
bool Configuration::GetDynamicCultures() const
{
   return dynamic_cultures_;
}
bool Configuration::GetDynamicReligions() const
{
   return dynamic_religions_;
}
std::string Configuration::GetTechSource() const
{
   return tech_source_;
}
std::string Configuration::GetArmyScale() const
{
   return army_scale_;
}
bool Configuration::GetTreasuryImport() const
{
   return treasury_import_;
}
void Configuration::SetDynamicCultures(bool dynamic_cultures)
{
   dynamic_cultures_ = dynamic_cultures;
}
void Configuration::SetDynamicReligions(bool dynamic_religions)
{
   dynamic_religions_ = dynamic_religions;
}
void Configuration::SetTechSource(const std::string& source)
{
   tech_source_ = source;
}
void Configuration::SetArmyScale(const std::string& scale)
{
   army_scale_ = scale;
}
void Configuration::SetTreasuryImport(bool treasury_import)
{
   treasury_import_ = treasury_import;
}
bool Configuration::GetWarImport() const
{
   return war_import_;
}
void Configuration::SetWarImport(bool war_import)
{
   war_import_ = war_import;
}

void Configuration::Validate(const commonItems::ConverterVersion& converter_version) const
{
   VerifyCK3Path();
   VerifyCK3Version(converter_version);
   VerifyEU5Path();
   VerifyEU5Version(converter_version);
   VerifyCK3Save();
}

void Configuration::VerifyCK3Path() const
{
   if (!commonItems::DoesFolderExist(ck3_directory_))
   {
      throw std::runtime_error(fmt::format("Crusader Kings 3 path {} doesn't exist.", ck3_directory_.string()));
   }
   if (!commonItems::DoesFileExist(ck3_directory_ / "binaries/ck3.exe") &&
       !commonItems::DoesFileExist(ck3_directory_ / "CK3game") &&
       !commonItems::DoesFileExist(ck3_directory_ / "binaries/ck3"))
   {
      throw std::runtime_error(fmt::format("{} does not contain Crusader Kings 3.", ck3_directory_.string()));
   }
}

void Configuration::VerifyEU5Path() const
{
   if (!commonItems::DoesFolderExist(eu5_directory_))
   {
      throw std::runtime_error(fmt::format("Europa Universalis 5 path {} doesn't exist.", eu5_directory_.string()));
   }
   if (!commonItems::DoesFileExist(eu5_directory_ / "binaries/eu5.exe") &&
       !commonItems::DoesFileExist(eu5_directory_ / "binaries/eu5"))
   {
      throw std::runtime_error(fmt::format("{} does not contain Europa Universalis 5.", eu5_directory_.string()));
   }
}

void Configuration::VerifyCK3Version(const commonItems::ConverterVersion& converter_version) const
{
   const auto ck3_version = GameVersion::extractVersionFromLauncher(ck3_directory_ / "launcher/launcher-settings.json");
   if (!ck3_version)
   {
      Log(LogLevel::Error) << "CK3 version could not be determined, proceeding blind!";
      return;
   }

   Log(LogLevel::Info) << "CK3 version: " << ck3_version->toShortString();

   if (converter_version.getMinSource() > *ck3_version)
   {
      Log(LogLevel::Error) << "CK3 version is v" << ck3_version->toShortString() << ", converter requires minimum v"
                           << converter_version.getMinSource().toShortString() << "!";
      throw std::runtime_error("Converter vs CK3 installation mismatch!");
   }

   if (!converter_version.getMaxSource().isLargerishThan(*ck3_version))
   {
      Log(LogLevel::Error) << "CK3 version is v" << ck3_version->toShortString() << ", converter requires maximum v"
                           << converter_version.getMaxSource().toShortString() << "!";
      throw std::runtime_error("Converter vs CK3 installation mismatch!");
   }
}

void Configuration::VerifyEU5Version(const commonItems::ConverterVersion& converter_version) const
{
   (void)converter_version;

   // EU5 ships no launcher-settings.json, so there is no version number to compare against. The
   // checksum identifies the build well enough for bug reports, and the layout check below is what
   // actually matters: the converter reads vanilla setup files directly, so a patch that moves or
   // renames them breaks conversion in ways that are much easier to diagnose here than later.
   if (const auto checksum_file = eu5_directory_ / "binaries/checksum.txt"; commonItems::DoesFileExist(checksum_file))
   {
      std::ifstream checksum_stream(checksum_file);
      std::string checksum;
      std::getline(checksum_stream, checksum);
      if (!checksum.empty())
      {
         Log(LogLevel::Info) << "EU5 build checksum: " << checksum;
      }
   }
   else
   {
      Log(LogLevel::Warning) << "EU5 build checksum could not be read; proceeding blind.";
   }

   static const std::vector<std::filesystem::path> required_data = {
       "game/main_menu/setup/start/10_countries.txt",
       "game/main_menu/setup/templates",
       "game/in_game/common/cultures",
       "game/in_game/common/religions",
       "game/in_game/map_data/definitions.txt",
   };
   std::vector<std::string> missing;
   for (const auto& entry: required_data)
   {
      const auto full_path = eu5_directory_ / entry;
      if (!commonItems::DoesFileExist(full_path) && !commonItems::DoesFolderExist(full_path))
      {
         missing.push_back(entry.string());
      }
   }
   if (!missing.empty())
   {
      for (const auto& entry: missing)
      {
         Log(LogLevel::Error) << "Missing EU5 game data: " << entry;
      }
      throw std::runtime_error(
          "This Europa Universalis 5 installation is missing game data the converter reads. It is "
          "probably a newer version than this converter supports.");
   }
}

void Configuration::VerifyCK3Save() const
{
   if (save_game_.extension() != ".ck3")
   {
      throw std::invalid_argument(
          "The save is not a Crusader Kings 3 save. Choose a save ending in '.ck3' and convert again.");
   }
}


}  // namespace configuration
