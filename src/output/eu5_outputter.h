#pragma once

#include "config/configuration.h"
#include "diagnostics/diagnostics_report.h"
#include "eu5/framework.h"
#include "eu5/world.h"

namespace ck3eu5::output {

class Eu5Outputter
{
  public:
	void write(const eu5::World& world,
		 const eu5::WorldFramework& framework,
		 const config::Configuration& configuration,
		 const diagnostics::DiagnosticsReport& diagnostics) const;
};

}  // namespace ck3eu5::output
