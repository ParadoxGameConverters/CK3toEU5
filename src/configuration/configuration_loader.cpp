#include "src/configuration/configuration_loader.hpp"

#include <external/commonItems/CommonFunctions.h>
#include <external/commonItems/Log.h>
#include <external/commonItems/Parser.h>
#include <external/commonItems/ParserHelpers.h>

#include <filesystem>
#include <iostream>
#include <string>

#include "configuration.hpp"


using std::filesystem::path;



namespace
{

std::string DetermineOutputName(const path& save_path)
{
   return save_path.stem().string();
}

std::string EnsureOutputNameNotEmpty(const std::string& output_name, const path& save_path)
{
   if (output_name.empty())
   {
      Log(LogLevel::Info) << "\tOutput name in config empty, using save name instead.";
      return DetermineOutputName(save_path);
   }
   return output_name;
}

}  // namespace


configuration::Configuration configuration::LoadConfiguration(const path& configuration_file)
{
   commonItems::parser configuration_parser;
   Configuration configuration;

   configuration_parser.registerKeyword("CK3DocDirectory", [&configuration](std::istream& stream) {
      configuration.SetCK3DocDirectory(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tCrusader Kings 3 documents directory is " << configuration.GetCK3DocDirectory();
   });
   configuration_parser.registerKeyword("CK3directory", [&configuration](std::istream& stream) {
      configuration.SetCK3Directory(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tCrusader Kings 3 install path is " << configuration.GetCK3Directory();
   });
   configuration_parser.registerKeyword("EU5directory", [&configuration](std::istream& stream) {
      configuration.SetEU5Directory(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tEuropa Universalis 5 install path is " << configuration.GetEU5Directory();
   });
   configuration_parser.registerKeyword("targetGameModPath", [&configuration](std::istream& stream) {
      configuration.SetEU5ModPath(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tEuropa Universalis 5 mod path is " << configuration.GetEU5ModPath();
   });
   configuration_parser.registerKeyword("SaveGame", [&configuration](std::istream& stream) {
      configuration.SetSaveGamePath(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tSave game is " << configuration.GetSaveGamePath();
   });
   configuration_parser.registerKeyword("debug", [&configuration](std::istream& stream) {
      configuration.SetDebug(commonItems::getString(stream) == "yes");
      if (configuration.GetDebug())
      {
         Log(LogLevel::Info) << "\tDebug is active";
      }
   });
   configuration_parser.registerKeyword("output_name", [&configuration](std::istream& stream) {
      configuration.SetOutputName(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tOutput name given in config is " << configuration.GetOutputName();
   });
   configuration_parser.registerKeyword("shatter_empires", [&configuration](std::istream& stream) {
      configuration.SetShatterEmpires(commonItems::getString(stream) == "yes");
      Log(LogLevel::Info) << "\tShatter empires: " << (configuration.GetShatterEmpires() ? "yes" : "no");
   });
   configuration_parser.registerKeyword("vassal_splitoff", [&configuration](std::istream& stream) {
      configuration.SetVassalSplitoff(commonItems::getString(stream) == "yes");
      Log(LogLevel::Info) << "\tVassal splitoff: " << (configuration.GetVassalSplitoff() ? "yes" : "no");
   });
   configuration_parser.registerKeyword("hre_mode", [&configuration](std::istream& stream) {
      configuration.SetHREMode(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tHRE mode: " << configuration.GetHREMode();
   });
   configuration_parser.registerKeyword("dev_import", [&configuration](std::istream& stream) {
      configuration.SetDevImport(commonItems::getString(stream) != "no");
      Log(LogLevel::Info) << "\tDevelopment import: " << (configuration.GetDevImport() ? "yes" : "no");
   });
   configuration_parser.registerKeyword("dynamic_cultures", [&configuration](std::istream& stream) {
      configuration.SetDynamicCultures(commonItems::getString(stream) != "no");
      Log(LogLevel::Info) << "\tDynamic cultures: " << (configuration.GetDynamicCultures() ? "yes" : "no");
   });
   configuration_parser.registerKeyword("dynamic_religions", [&configuration](std::istream& stream) {
      configuration.SetDynamicReligions(commonItems::getString(stream) != "no");
      Log(LogLevel::Info) << "\tDynamic religions: " << (configuration.GetDynamicReligions() ? "yes" : "no");
   });
   configuration_parser.registerKeyword("tech_source", [&configuration](std::istream& stream) {
      configuration.SetTechSource(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tTechnology source: " << configuration.GetTechSource();
   });
   configuration_parser.registerKeyword("army_scale", [&configuration](std::istream& stream) {
      configuration.SetArmyScale(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tArmy scale: " << configuration.GetArmyScale();
   });
   configuration_parser.registerKeyword("treasury_import", [&configuration](std::istream& stream) {
      configuration.SetTreasuryImport(commonItems::getString(stream) != "no");
      Log(LogLevel::Info) << "\tTreasury import: " << (configuration.GetTreasuryImport() ? "yes" : "no");
   });
   configuration_parser.registerKeyword("war_import", [&configuration](std::istream& stream) {
      configuration.SetWarImport(commonItems::getString(stream) != "no");
      Log(LogLevel::Info) << "\tWar import: " << (configuration.GetWarImport() ? "yes" : "no");
   });

   configuration_parser.parseFile(configuration_file);

   configuration.SetOutputName(
       normalizeStringPath(EnsureOutputNameNotEmpty(configuration.GetOutputName(), configuration.GetSaveGamePath())));
   Log(LogLevel::Info) << "\tUsing output name " << configuration.GetOutputName();


   return configuration;
}