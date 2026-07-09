#include "src/configuration/configuration_loader.hpp"

#include <external/commonItems/CommonFunctions.h>
#include <external/commonItems/Log.h>
#include <external/commonItems/OSCompatibilityLayer.h>
#include <external/commonItems/ParserHelpers.h>
#include <external/fmt/include/fmt/format.h>



using std::filesystem::path;



namespace
{

std::string DetermineOutputName(const path& save_name)
{
   if (save_name.extension() != ".ck3")
   {
      throw std::invalid_argument(
          "The save is not a Crusader Kings 3 save. Choose a save ending in '.ck3' and convert again.");
   }

   return save_name.stem().string();
}

}  // namespace


configuration::Configuration configuration::LoadConfiguration(const path& configuration_file,
    const commonItems::ConverterVersion& converter_version)
{
   commonItems::parser configuration_parser;
   Configuration configuration;

   configuration_parser.registerKeyword("CK3DocDirectory", [&configuration](std::istream& stream) {
      configuration.ck3_doc_directory = commonItems::getString(stream);
      Log(LogLevel::Info) << "\tCrusader Kings 3 documents directory is " << configuration.ck3_doc_directory;
   });
   configuration_parser.registerKeyword("CK3directory", [&configuration, &converter_version](std::istream& stream) {
      configuration.ck3_directory = commonItems::getString(stream);
      Log(LogLevel::Info) << "\tCrusader Kings 3 install path is " << configuration.ck3_directory;
      configuration.VerifyCK3Path();
      configuration.VerifyCK3Version(converter_version);
   });
   configuration_parser.registerKeyword("EU5directory", [&configuration, &converter_version](std::istream& stream) {
      configuration.eu5_directory = commonItems::getString(stream);
      Log(LogLevel::Info) << "\tEuropa Universalis 5 install path is " << configuration.eu5_directory;
      configuration.VerifyEU5Path();
      configuration.VerifyEU5Version(converter_version);
   });
   configuration_parser.registerKeyword("targetGameModPath", [&configuration](std::istream& stream) {
      configuration.eu5_mod_path = commonItems::getString(stream);
      Log(LogLevel::Info) << "\tEuropa Universalis 5 mod path is " << configuration.eu5_mod_path;
   });
   configuration_parser.registerKeyword("SaveGame", [&configuration](std::istream& stream) {
      configuration.save_game = commonItems::getString(stream);
      Log(LogLevel::Info) << "\tSave game is " << configuration.save_game;
   });
   configuration_parser.registerKeyword("debug", [&configuration](std::istream& stream) {
      configuration.debug = commonItems::getString(stream) == "yes";
      if (configuration.debug)
      {
         Log(LogLevel::Info) << "\tDebug is active";
      }
      else
      {
         Log(LogLevel::Info) << "\tDebug is not active";
      }
   });
   configuration_parser.registerKeyword("output_name", [&configuration](std::istream& stream) {
      configuration.output_name = commonItems::getString(stream);
      Log(LogLevel::Info) << "\tOutput name is " << configuration.output_name;
   });

   configuration_parser.parseFile(configuration_file);

   if (configuration.output_name.empty())
   {
      configuration.output_name = DetermineOutputName(configuration.save_game);
   }
   configuration.output_name = normalizeStringPath(configuration.output_name);

   Log(LogLevel::Info) << "Using output name " << configuration.output_name;

   return configuration;
}