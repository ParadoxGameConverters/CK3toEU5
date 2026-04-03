#pragma once

#include "diagnostics/diagnostics_report.h"
#include "eu5/installed_definitions.h"

#include <filesystem>

namespace ck3eu5::eu5 {

class InstalledDefinitionsLoader
{
  public:
	[[nodiscard]] InstalledDefinitions load(const std::filesystem::path& game_path,
		 diagnostics::DiagnosticsReport& diagnostics) const;
};

}  // namespace ck3eu5::eu5
