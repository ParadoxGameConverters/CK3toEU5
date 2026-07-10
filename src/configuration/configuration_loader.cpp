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

std::string DetermineOutputName(const path& save_path)  // NOLINT : it says they should be static instead, when made
                                                        // static it says they should be in anonymous namespace instead
{
   return save_path.stem().string();
}

std::string EnsureOutputNameNotEmpty(const std::string& output_name,
    const path& save_path)  // NOLINT : it says they should be static instead, when made static it says they should be
                            // in anonymous namespace instead
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
      else
      {
         Log(LogLevel::Info) << "\tDebug is not active";
      }
   });
   configuration_parser.registerKeyword("output_name", [&configuration](std::istream& stream) {
      configuration.SetOutputName(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tOutput name given in config is " << configuration.GetOutputName();
   });

   configuration_parser.parseFile(configuration_file);

   configuration.SetOutputName(
       normalizeStringPath(EnsureOutputNameNotEmpty(configuration.GetOutputName(), configuration.GetSaveGamePath())));
   Log(LogLevel::Info) << "\tUsing output name " << configuration.GetOutputName();


   return configuration;
}