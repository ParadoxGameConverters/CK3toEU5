#ifndef CONFIGURATION_CONFIGURATION_H
#define CONFIGURATION_CONFIGURATION_H



#include <external/commonItems/ConverterVersion.h>
#include <external/commonItems/OSCompatibilityLayer.h>
#include <external/fmt/include/fmt/format.h>

#include <filesystem>
#include <string>



namespace configuration
{

struct Configuration
{
   std::filesystem::path ck3_directory;
   std::filesystem::path ck3_doc_directory;
   std::filesystem::path eu5_directory;
   std::filesystem::path eu5_mod_path;
   std::filesystem::path save_game;
   bool debug = false;
   std::string output_name;

   void VerifyCK3Path() const
   {
      if (!commonItems::DoesFolderExist(ck3_directory))
      {
         throw std::runtime_error(fmt::format("Crusader Kings 3 path {} doesn't exist.", ck3_directory.string()));
      }
      if (!commonItems::DoesFileExist(ck3_directory / "binaries/ck3.exe") &&
          !commonItems::DoesFileExist(ck3_directory / "CK3game") &&
          !commonItems::DoesFileExist(ck3_directory / "binaries/ck3"))
      {
         throw std::runtime_error(fmt::format("{} does not contain Crusader Kings 3.", ck3_directory.string()));
      }
   }

   void VerifyEU5Path() const
   {
      if (!commonItems::DoesFolderExist(eu5_directory))
      {
         throw std::runtime_error(fmt::format("Europa Universalis 5 path {} doesn't exist.", eu5_directory.string()));
      }
      if (!commonItems::DoesFileExist(eu5_directory / "binaries/eu5.exe") &&
          !commonItems::DoesFileExist(eu5_directory / "binaries/eu5"))
      {
         throw std::runtime_error(fmt::format("{} does not contain Europa Universalis 5.", eu5_directory.string()));
      }
   }

   void VerifyCK3Version(const commonItems::ConverterVersion& converter_version) const
   {
      const auto ck3_version =
          GameVersion::extractVersionFromLauncher(ck3_directory / "launcher/launcher-settings.json");
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
      const auto eu5_version = GameVersion::extractVersionFromBranchTxt(eu5_directory / "clausewitz_branch.txt");
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
};

const Configuration kDefaultConfig = {};
const Configuration kDebugConfig = {.debug = true};


}  // namespace configuration


#endif  // SRC_CONFIGURATION_CONFIGURATION_H#pragma once
