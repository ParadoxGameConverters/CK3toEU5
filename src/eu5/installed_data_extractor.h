#pragma once

#include "diagnostics/diagnostics_report.h"
#include "eu5/framework.h"

#include <filesystem>

namespace ck3eu5::eu5 {

class InstalledDataExtractor
{
  public:
	[[nodiscard]] WorldFramework extract(const std::filesystem::path& game_path,
		 diagnostics::DiagnosticsReport& diagnostics) const;

	void writeCsvs(const WorldFramework& framework,
		 const std::filesystem::path& location_framework_path,
		 const std::filesystem::path& country_colors_path) const;
};

}  // namespace ck3eu5::eu5
