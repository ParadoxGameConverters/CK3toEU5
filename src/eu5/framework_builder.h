#pragma once

#include "config/configuration.h"
#include "diagnostics/diagnostics_report.h"
#include "eu5/framework.h"

namespace ck3eu5::eu5 {

class WorldFrameworkBuilder
{
  public:
	WorldFramework load(const config::Configuration& configuration, diagnostics::DiagnosticsReport& diagnostics) const;
};

}  // namespace ck3eu5::eu5
