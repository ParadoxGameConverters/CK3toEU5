#include "ck3_to_eu5_converter.hpp"

#include <external/commonItems/ConverterVersion.h>
#include <external/commonItems/Log.h>

#include <utility>

#include "configuration/configuration.hpp"
#include "output/output_mod.hpp"


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

   // auto output_writer = out::OutputWriter(configuration_.GetOutputName(), converter_version_.getMaxTarget());
   // Log(LogLevel::Info) << "Preparing output folder";
   // output_writer.ClearOutputFolder();
   // output_writer.CreateOutputFolder();
   Log(LogLevel::Info) << "Outputting mod";
   out::OutputWriter output = out::OutputWriter(configuration_.GetOutputName(), converter_version_.getMaxTarget());
   output.BuildFolderStructure();
   output.GenereteOutputMod();

   Log(LogLevel::Progress) << "85%";
}

}  // namespace ck3_to_eu5