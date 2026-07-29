#include "configuration.hpp"

#include <external/commonItems/ConverterVersion.h>
#include <external/commonItems/GameVersion.h>
#include <external/commonItems/Log.h>
#include <external/commonItems/OSCompatibilityLayer.h>

#include <filesystem>
#include <format>
#include <stdexcept>
#include <string>
#include <utility>



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
      throw std::runtime_error(std::format("Crusader Kings 3 path {} doesn't exist.", ck3_directory_.string()));
   }
   if (!commonItems::DoesFileExist(ck3_directory_ / "binaries/ck3.exe") &&
       !commonItems::DoesFileExist(ck3_directory_ / "CK3game") &&
       !commonItems::DoesFileExist(ck3_directory_ / "binaries/ck3"))
   {
      throw std::runtime_error(std::format("{} does not contain Crusader Kings 3.", ck3_directory_.string()));
   }
}

void Configuration::VerifyEU5Path() const
{
   if (!commonItems::DoesFolderExist(eu5_directory_))
   {
      throw std::runtime_error(std::format("Europa Universalis 5 path {} doesn't exist.", eu5_directory_.string()));
   }
   if (!commonItems::DoesFileExist(eu5_directory_ / "binaries/eu5.exe") &&
       !commonItems::DoesFileExist(eu5_directory_ / "binaries/eu5"))
   {
      throw std::runtime_error(std::format("{} does not contain Europa Universalis 5.", eu5_directory_.string()));
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

void Configuration::VerifyEU5Version(  // NOLINT: not yet implemented
    const commonItems::ConverterVersion& converter_version) const
{
   // TODO(kubkm): - find a way to get eu5 version
   (void)converter_version;
   Log(LogLevel::Error) << "EU5 version could not be determined, proceeding blind!";
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
