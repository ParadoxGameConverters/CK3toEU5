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
   [[nodiscard]] bool GetShatterEmpires() const;
   [[nodiscard]] bool GetVassalSplitoff() const;
   [[nodiscard]] std::string GetHREMode() const;
   [[nodiscard]] bool GetDevImport() const;
   [[nodiscard]] bool GetDynamicCultures() const;
   [[nodiscard]] bool GetDynamicReligions() const;
   [[nodiscard]] std::string GetTechSource() const;
   [[nodiscard]] std::string GetArmyScale() const;
   [[nodiscard]] bool GetTreasuryImport() const;
   [[nodiscard]] bool GetWarImport() const;

   void SetCK3Directory(std::filesystem::path ck3_dir);
   void SetCK3DocDirectory(std::filesystem::path ck3_doc_dir);
   void SetEU5Directory(std::filesystem::path eu5_dir);
   void SetEU5ModPath(std::filesystem::path eu5_mod_p);
   void SetSaveGamePath(std::filesystem::path save_game_p);
   void SetDebug(bool debug);
   void SetOutputName(const std::string& name);
   void SetShatterEmpires(bool shatter);
   void SetVassalSplitoff(bool splitoff);
   void SetHREMode(const std::string& mode);
   void SetDevImport(bool dev_import);
   void SetDynamicCultures(bool dynamic_cultures);
   void SetDynamicReligions(bool dynamic_religions);
   void SetTechSource(const std::string& source);
   void SetArmyScale(const std::string& scale);
   void SetTreasuryImport(bool treasury_import);
   void SetWarImport(bool war_import);

   void Validate(const commonItems::ConverterVersion& converter_version) const;

  private:
   std::filesystem::path ck3_directory_;
   std::filesystem::path ck3_doc_directory_;
   std::filesystem::path eu5_directory_;
   std::filesystem::path eu5_mod_path_;
   std::filesystem::path save_game_;
   bool debug_ = false;
   std::string output_name_;
   bool shatter_empires_ = false;    // split empire-tier realms into their kingdom vassals
   bool vassal_splitoff_ = false;    // convert kingdom-tier vassals as subject countries
   std::string hre_mode_ = "io";     // io: write an hre international organization; none: skip
   bool dev_import_ = true;          // apply CK3 development bonuses to locations
   bool dynamic_cultures_ = true;    // write hybrid/divergent CK3 cultures as new EU5 cultures
   bool dynamic_religions_ = true;   // write reformed/custom CK3 faiths as new EU5 religions
   std::string tech_source_ = "ck3"; // ck3: technology from the ruling culture's era; vanilla: from the setup template
   std::string army_scale_ = "normal";  // small/normal/large multiplier on converted standing armies
   bool treasury_import_ = true;     // start countries with the CK3 ruler's personal gold
   bool war_import_ = true;          // continue active CK3 wars in EU5 (with wargoals); off = everyone starts at peace



   void VerifyCK3Path() const;

   void VerifyEU5Path() const;

   void VerifyCK3Version(const commonItems::ConverterVersion& converter_version) const;

   void VerifyEU5Version(const commonItems::ConverterVersion& converter_version) const;

   void VerifyCK3Save() const;
};


}  // namespace configuration


#endif  // SRC_CONFIGURATION_CONFIGURATION_H#pragma once
