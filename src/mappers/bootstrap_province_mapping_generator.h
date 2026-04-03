#pragma once

#include "ck3/installed_titles.h"
#include "diagnostics/diagnostics_report.h"
#include "eu5/framework.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace ck3eu5::mappers {

struct BootstrapProvinceMapping
{
	std::string ck3_county;
	std::string county_display_name;
	std::string duchy_key;
	std::string kingdom_key;
	std::string empire_key;
	std::string primary_barony_key;
	std::string primary_barony_display_name;
	std::vector<std::string> eu5_locations;
	std::vector<std::string> sources;
};

struct BootstrapProvinceMappingResult
{
	size_t total_counties = 0;
	size_t manual_counties = 0;
	size_t generated_counties = 0;
	size_t unmapped_counties = 0;
	std::map<std::string, size_t> source_counts;
	std::vector<BootstrapProvinceMapping> mappings;
};

class BootstrapProvinceMappingGenerator
{
  public:
	BootstrapProvinceMappingResult generate(const ck3::InstalledTitles& installed_titles,
		 const eu5::WorldFramework& framework,
		 const std::filesystem::path& existing_province_mappings_path,
		 diagnostics::DiagnosticsReport& diagnostics) const;

	void writeCsvs(const BootstrapProvinceMappingResult& result,
		 const std::filesystem::path& province_mappings_path,
		 const std::filesystem::path& report_path) const;
};

}  // namespace ck3eu5::mappers
