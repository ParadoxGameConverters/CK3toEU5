#pragma once

#include "ck3/installed_titles.h"
#include "diagnostics/diagnostics_report.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ck3eu5::maps {

struct CorrespondenceCandidate
{
	std::string eu5_province_definition;
	std::vector<std::string> eu5_locations;
	std::vector<std::string> eu5_display_names;
	uint64_t votes = 0;
	double vote_share = 0.0;
	double centroid_distance = 1.0;
};

struct CountyCorrespondence
{
	std::string ck3_county;
	std::string display_name;
	std::vector<std::string> barony_keys;
	std::vector<std::string> barony_display_names;
	std::vector<int> ck3_province_ids;
	uint64_t raster_pixels = 0;
	double centroid_x = 0.0;
	double centroid_y = 0.0;
	std::vector<CorrespondenceCandidate> candidates;
};

struct CorrespondenceResult
{
	size_t total_counties = 0;
	size_t counties_with_any_pixels = 0;
	size_t counties_with_candidates = 0;
	std::vector<CountyCorrespondence> counties;
};

class MapCorrespondenceBuilder
{
  public:
	CorrespondenceResult build(const std::filesystem::path& ck3_game_path,
		 const std::filesystem::path& eu5_game_path,
		 const std::filesystem::path& control_province_mappings_path,
		 const std::filesystem::path& location_framework_path,
		 diagnostics::DiagnosticsReport& diagnostics) const;

	void writeCandidatesCsv(const CorrespondenceResult& result, const std::filesystem::path& path) const;
	void writeTopMappingsCsv(const CorrespondenceResult& result,
		 const std::filesystem::path& path,
		 double minimum_vote_share = 0.50,
		 double maximum_centroid_distance = 0.08) const;
	void writeAugmentedMappingsCsv(const std::filesystem::path& base_mappings_path,
		 const CorrespondenceResult& result,
		 const std::filesystem::path& path,
		 double minimum_vote_share = 0.50,
		 double maximum_centroid_distance = 0.08) const;
};

}  // namespace ck3eu5::maps
