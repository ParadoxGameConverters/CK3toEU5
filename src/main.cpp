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
   }
   catch (const std::exception& e)
   {
      Log(LogLevel::Error) << e.what();
      return -1;
   }

   return 0;
}