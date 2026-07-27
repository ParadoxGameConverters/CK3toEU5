#ifndef SRC_CK3TOEU5CONVERTER_H
#define SRC_CK3TOEU5CONVERTER_H

#include <external/commonItems/ConverterVersion.h>

#include "src/configuration/configuration.hpp"

namespace ck3_to_eu5
{

void ConvertCk3ToEu5(const configuration::Configuration& configuration, const commonItems::ConverterVersion& converter_version);

}  // namespace ck3_to_eu5

#endif  // SRC_CK3TOEU5CONVERTER_H
