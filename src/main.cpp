#include <external/commonItems/ConverterVersion.h>
#include <external/commonItems/Log.h>

#include <exception>
#include <print>

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

      // Meet C++23's new hotness!
      std::println("Hello, world!");
      ck3_to_eu5::ConvertCk3ToEu5();
   }
   catch (const std::exception& e)
   {
      Log(LogLevel::Error) << e.what();
      return -1;
   }

   return 0;
}