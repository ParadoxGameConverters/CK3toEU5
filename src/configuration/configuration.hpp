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
   std::filesystem::path get_ck3_directory() const { return ck3_directory; }
   std::filesystem::path get_ck3_doc_directory() const { return ck3_doc_directory; }
   std::filesystem::path get_eu5_directory() const { return eu5_directory; }
   std::filesystem::path get_eu5_mod_path() const { return eu5_mod_path; }
   std::filesystem::path get_save_game_path() const { return save_game; }
   bool get_debug() const { return debug; }
   std::string get_output_name() const { return output_name; }

   void set_ck3_directory(std::filesystem::path ck3_dir) { ck3_directory = ck3_dir; }
   void set_ck3_doc_directory(std::filesystem::path ck3_doc_dir) { ck3_doc_directory = ck3_doc_dir; }
   void set_eu5_directory(std::filesystem::path eu5_dir) { eu5_directory = eu5_dir; }
   void set_eu5_mod_path(std::filesystem::path eu5_mod_p) { eu5_mod_path = eu5_mod_p; }
   void set_save_game_path(std::filesystem::path save_game_p) { save_game = save_game_p; }
   void set_debug(bool d) { debug = d; }
   void set_output_name(const std::string& name) { output_name = name; }

   void validate(const commonItems::ConverterVersion& converter_version) const
   {
      verifyCK3Path();
      verifyCK3Version(converter_version);
      verifyEU5Path();
      verifyEU5Version(converter_version);
      verifyCK3Save();
   }

  private:
   std::filesystem::path ck3_directory;
   std::filesystem::path ck3_doc_directory;
   std::filesystem::path eu5_directory;
   std::filesystem::path eu5_mod_path;
   std::filesystem::path save_game;
   bool debug = false;
   std::string output_name;



   void verifyCK3Path() const
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

   void verifyEU5Path() const
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

   void verifyCK3Version(const commonItems::ConverterVersion& converter_version) const
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

   void verifyEU5Version(const commonItems::ConverterVersion& converter_version) const
   {
      // TODO find a way to get eu5 version
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

   void verifyCK3Save() const
   {
      if (save_game.extension() != ".ck3")
      {
         throw std::invalid_argument(
             "The save is not a Crusader Kings 3 save. Choose a save ending in '.ck3' and convert again.");
      }
   }
};


}  // namespace configuration


#endif  // SRC_CONFIGURATION_CONFIGURATION_H#pragma once
