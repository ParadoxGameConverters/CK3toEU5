#include <external/commonItems/ConverterVersion.h>
#include <external/commonItems/Log.h>

#include <exception>

#include "ck3_to_eu5_converter.hpp"
#include "src/configuration/configuration.hpp"
#include "src/configuration/configuration_loader.hpp"



int main()  // NOLINT(bugprone-exception-escape)
{
   try
   {
      Log(LogLevel::Progress) << "0%";

      commonItems::ConverterVersion converter_version;
      converter_version.loadVersion("../version.txt");
      Log(LogLevel::Info) << converter_version;

      const auto configuration = configuration::LoadConfiguration("configuration.txt");
      configuration.Validate(converter_version);
      Log(LogLevel::Progress) << "3%";
      Log(LogLevel::Info) << "Converter configuration valid, starting conversion";

      auto converter = ck3_to_eu5::Converter(configuration, converter_version);
      converter.Convert();
      Log(LogLevel::Progress) << "100%";
      Log(LogLevel::Notice) << "* Conversion complete *";
      // EU5 has no launcher to register the mod with, so the frontend cannot switch it on for
      // the player. Say how, or the last step of a conversion is left for them to discover.
      Log(LogLevel::Notice) << "Once the mod has been copied, start EU5, open Mods & DLCs from the main menu and turn on "
                            << configuration.GetOutputName() << ".";
   }
   catch (const std::exception& e)
   {
      Log(LogLevel::Error) << e.what();
      return -1;
   }

   return 0;
}