#pragma once

#include "ck3/world.h"
#include "config/configuration.h"
#include "diagnostics/diagnostics_report.h"
#include "eu5/framework.h"
#include "eu5/world.h"
#include "mappers/mapper_bundle.h"

namespace ck3eu5::convert {

class WorldConverter
{
  public:
	eu5::World convert(const ck3::World& ck3_world,
		 const eu5::WorldFramework& framework,
		 const mappers::MapperBundle& mappers,
		 const config::Configuration& configuration,
		 diagnostics::DiagnosticsReport& diagnostics) const;
};

}  // namespace ck3eu5::convert
