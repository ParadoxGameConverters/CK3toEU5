#pragma once

#include "config/configuration.h"
#include "diagnostics/diagnostics_report.h"
#include "eu5/framework.h"
#include "eu5/installed_definitions.h"
#include "eu5/world.h"

namespace ck3eu5::eu5 {

class WorldSanitizer
{
  public:
	void sanitize(World& world,
		 const WorldFramework& framework,
		 const config::Configuration& configuration,
		 const InstalledDefinitions& definitions,
		 diagnostics::DiagnosticsReport& diagnostics) const;
};

}  // namespace ck3eu5::eu5
