#include "ck3_to_eu5_converter.hpp"

#include <external/commonItems/ConverterVersion.h>
#include <external/commonItems/Log.h>

#include <filesystem>
#include <utility>

#include "ck3_world/ck3_world.hpp"
#include "configuration/configuration.hpp"
#include "eu5_world/eu5_world.hpp"
#include "mappers/mappers.hpp"
#include "output/output.hpp"


namespace ck3_to_eu5
{

Converter::Converter(configuration::Configuration configuration, commonItems::ConverterVersion converter_version):
    configuration_(std::move(configuration)),
    converter_version_(std::move(converter_version))
{
}

void Converter::Convert()
{
   Log(LogLevel::Progress) << "5%";
   const ck3::CK3World ck3_world(configuration_, converter_version_);

   Log(LogLevel::Progress) << "50%";

   Log(LogLevel::Info) << "-> Loading mappings.";
   const mappers::Mappers mappers(std::filesystem::path("configurables"));
   mappers.LogCoverageReport(ck3_world.GetRealms(),
       ck3_world.GetReligions(),
       ck3_world.GetCultures(),
       ck3_world.GetLandedTitles());

   Log(LogLevel::Info) << "-> Building EU5 countries.";
   const eu5::GameDefinitions game_definitions(configuration_.GetEU5Directory());
   const eu5::LocationData location_data(configuration_.GetEU5Directory());
   const eu5::EU5World eu5_world(ck3_world, mappers, game_definitions, location_data);
   eu5_world.LogReport();

   Log(LogLevel::Progress) << "80%";


   Log(LogLevel::Info) << "Outputting mod";
   const eu5::VanillaCountries vanilla_countries(configuration_.GetEU5Directory());
   out::Output output =
       out::Output(configuration_.GetOutputName(), converter_version_, eu5_world, location_data, vanilla_countries, configuration_.GetEU5Directory());
   output.GenerateOutputMod();

   Log(LogLevel::Progress) << "85%";
}

}  // namespace ck3_to_eu5