#pragma once

#include "diagnostics/diagnostics_report.h"
#include "eu5/installed_definitions.h"
#include "eu5/world.h"

namespace ck3eu5::eu5 {

class WorldValidator
{
  public:
	void validate(const World& world, const InstalledDefinitions& definitions, diagnostics::DiagnosticsReport& diagnostics) const;
};

}  // namespace ck3eu5::eu5
