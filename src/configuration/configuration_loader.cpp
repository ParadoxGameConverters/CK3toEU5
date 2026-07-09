#include "src/configuration/configuration_loader.hpp"

#include <external/commonItems/CommonFunctions.h>
#include <external/commonItems/Log.h>
#include <external/commonItems/OSCompatibilityLayer.h>
#include <external/commonItems/ParserHelpers.h>
#include <external/fmt/include/fmt/format.h>



using std::filesystem::path;



namespace
{

std::string determineOutputName(const path& save_path)
{
   return save_path.stem().string();
}

std::string ensureOutputNameNotEmpty(const std::string& output_name, const path& save_path)
{
   if (output_name.empty())
   {
      Log(LogLevel::Info) << "\tOutput name in config empty, using save name instead.";
      return determineOutputName(save_path);
   }
   return output_name;
}

}  // namespace


configuration::Configuration configuration::LoadConfiguration(const path& configuration_file)
{
   commonItems::parser configuration_parser;
   Configuration configuration;

   configuration_parser.registerKeyword("CK3DocDirectory", [&configuration](std::istream& stream) {
      configuration.set_ck3_doc_directory(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tCrusader Kings 3 documents directory is " << configuration.get_ck3_doc_directory();
   });
   configuration_parser.registerKeyword("CK3directory", [&configuration](std::istream& stream) {
      configuration.set_ck3_directory(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tCrusader Kings 3 install path is " << configuration.get_ck3_directory();
   });
   configuration_parser.registerKeyword("EU5directory", [&configuration](std::istream& stream) {
      configuration.set_eu5_directory(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tEuropa Universalis 5 install path is " << configuration.get_eu5_directory();
   });
   configuration_parser.registerKeyword("targetGameModPath", [&configuration](std::istream& stream) {
      configuration.set_eu5_mod_path(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tEuropa Universalis 5 mod path is " << configuration.get_eu5_mod_path();
   });
   configuration_parser.registerKeyword("SaveGame", [&configuration](std::istream& stream) {
      configuration.set_save_game_path(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tSave game is " << configuration.get_save_game_path();
   });
   configuration_parser.registerKeyword("debug", [&configuration](std::istream& stream) {
      configuration.set_debug(commonItems::getString(stream) == "yes");
      if (configuration.get_debug())
      {
         Log(LogLevel::Info) << "\tDebug is active";
      }
      else
      {
         Log(LogLevel::Info) << "\tDebug is not active";
      }
   });
   configuration_parser.registerKeyword("output_name", [&configuration](std::istream& stream) {
      configuration.set_output_name(commonItems::getString(stream));
      Log(LogLevel::Info) << "\tOutput name given in config is " << configuration.get_output_name();
   });

   configuration_parser.parseFile(configuration_file);

   configuration.set_output_name(normalizeStringPath(
       ensureOutputNameNotEmpty(configuration.get_output_name(), configuration.get_save_game_path())));
   Log(LogLevel::Info) << "\tUsing output name " << configuration.get_output_name();


   return configuration;
}