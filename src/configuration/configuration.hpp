#ifndef CONFIGURATION_CONFIGURATION_H
#define CONFIGURATION_CONFIGURATION_H



#include <external/commonItems/ConverterVersion.h>
#include <external/commonItems/OSCompatibilityLayer.h>
#include <external/fmt/include/fmt/format.h>

#include <filesystem>
#include <string>



namespace configuration
{

class Configuration
{
  public:
   [[nodiscard]] std::filesystem::path GetCK3Directory() const { return ck3_directory_; }
   [[nodiscard]] std::filesystem::path GetCK3DocDirectory() const { return ck3_doc_directory_; }
   [[nodiscard]] std::filesystem::path GetEU5Directory() const { return eu5_directory_; }
   [[nodiscard]] std::filesystem::path GetEU5ModPath() const { return eu5_mod_path_; }
   [[nodiscard]] std::filesystem::path GetSaveGamePath() const { return save_game_; }
   [[nodiscard]] bool GetDebug() const { return debug_; }
   [[nodiscard]] std::string GetOutputName() const { return output_name_; }

   void SetCK3Directory(std::filesystem::path ck3_dir) { ck3_directory_ = std::move(ck3_dir); }
   void SetCK3DocDirectory(std::filesystem::path ck3_doc_dir) {ck3_doc_directory_ = std::move(ck3_doc_dir); }
   void SetEU5Directory(std::filesystem::path eu5_dir) {eu5_directory_ = std::move(eu5_dir); }
   void SetEU5ModPath(std::filesystem::path eu5_mod_p) {eu5_mod_path_ = std::move(eu5_mod_p); }
   void SetSaveGamePath(std::filesystem::path save_game_p) {save_game_ = std::move(save_game_p); }
   void SetDebug(bool debug) { debug_ = debug; }
   void SetOutputName(const std::string& name) { output_name_ = name; }

   void Validate(const commonItems::ConverterVersion& converter_version) const
   {
      VerifyCK3Path();
      VerifyCK3Version(converter_version);
      VerifyEU5Path();
      VerifyEU5Version(converter_version);
      VerifyCK3Save();
   }

  private:
   std::filesystem::path ck3_directory_;
   std::filesystem::path ck3_doc_directory_;
   std::filesystem::path eu5_directory_;
   std::filesystem::path eu5_mod_path_;
   std::filesystem::path save_game_;
   bool debug_ = false;
   std::string output_name_;



   void VerifyCK3Path() const
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

   void VerifyEU5Path() const
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

   void VerifyCK3Version(const commonItems::ConverterVersion& converter_version) const
   {
      const auto ck3_version =
          GameVersion::extractVersionFromLauncher(ck3_directory_ / "launcher/launcher-settings.json");
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

   void VerifyEU5Version(const commonItems::ConverterVersion& converter_version) const
   {
      // TODO: kubkm - find a way to get eu5 version
      const auto eu5_version = GameVersion::extractVersionFromBranchTxt(eu5_directory_ / "clausewitz_branch.txt");
      if (!eu5_version)
      {
         Log(LogLevel::Error) << "EU5 version could not be determined, proceeding blind!";
         return;
      }

      Log(LogLevel::Info) << "EU5 version: " << eu5_version->toShortString();

      if (converter_version.getMinTarget() > *eu5_version)
      {
         Log(LogLevel::Error) << "EU5 version is v" << eu5_version->toShortString() << ", converter requires minimum v"
                              << converter_version.getMinTarget().toShortString() << "!";
         throw std::runtime_error("Converter vs EU5 installation mismatch!");
      }

      if (!converter_version.getMaxTarget().isLargerishThan(*eu5_version))
      {
         Log(LogLevel::Error) << "EU5 version is v" << eu5_version->toShortString() << ", converter requires maximum v"
                              << converter_version.getMaxTarget().toShortString() << "!";
         throw std::runtime_error("Converter vs EU5 installation mismatch!");
      }
   }

   void VerifyCK3Save() const
   {
      if (save_game_.extension() != ".ck3")
      {
         throw std::invalid_argument(
             "The save is not a Crusader Kings 3 save. Choose a save ending in '.ck3' and convert again.");
      }
   }
};


}  // namespace configuration


#endif  // SRC_CONFIGURATION_CONFIGURATION_H#pragma once
