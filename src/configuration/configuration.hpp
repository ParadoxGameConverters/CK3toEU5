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
   [[nodiscard]] std::filesystem::path GetCK3Directory() const;
   [[nodiscard]] std::filesystem::path GetCK3DocDirectory() const;
   [[nodiscard]] std::filesystem::path GetEU5Directory() const;
   [[nodiscard]] std::filesystem::path GetEU5ModPath() const;
   [[nodiscard]] std::filesystem::path GetSaveGamePath() const;
   [[nodiscard]] bool GetDebug() const;
   [[nodiscard]] std::string GetOutputName() const;

   void SetCK3Directory(std::filesystem::path ck3_dir);
   void SetCK3DocDirectory(std::filesystem::path ck3_doc_dir);
   void SetEU5Directory(std::filesystem::path eu5_dir);
   void SetEU5ModPath(std::filesystem::path eu5_mod_p);
   void SetSaveGamePath(std::filesystem::path save_game_p);
   void SetDebug(bool debug);
   void SetOutputName(const std::string& name);

   void Validate(const commonItems::ConverterVersion& converter_version) const;

  private:
   std::filesystem::path ck3_directory_;
   std::filesystem::path ck3_doc_directory_;
   std::filesystem::path eu5_directory_;
   std::filesystem::path eu5_mod_path_;
   std::filesystem::path save_game_;
   bool debug_ = false;
   std::string output_name_;



   void VerifyCK3Path() const;

   void VerifyEU5Path() const;

   void VerifyCK3Version(const commonItems::ConverterVersion& converter_version) const;

   void VerifyEU5Version(const commonItems::ConverterVersion& converter_version) const;

   void VerifyCK3Save() const;
};


}  // namespace configuration


#endif  // SRC_CONFIGURATION_CONFIGURATION_H#pragma once
