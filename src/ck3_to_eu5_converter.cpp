#include "ck3_to_eu5_converter.hpp"

#include <external/commonItems/ConverterVersion.h>
#include <external/commonItems/Log.h>

#include <utility>

#include "configuration/configuration.hpp"
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
   Log(LogLevel::Progress) << "80%";


   Log(LogLevel::Info) << "Outputting mod";
   out::Output output = out::Output(configuration_.GetOutputName(), converter_version_);
   output.GenereteOutputMod();

   Log(LogLevel::Progress) << "85%";
}

}  // namespace ck3_to_eu5