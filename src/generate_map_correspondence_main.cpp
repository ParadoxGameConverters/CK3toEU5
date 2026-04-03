#include "diagnostics/diagnostics_report.h"
#include "maps/map_correspondence.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
	if (argc < 6 || argc > 8)
	{
		std::cerr << "Usage: ck3_to_eu5_generate_map_correspondence <ck3_game_path> <eu5_game_path> <control_province_mappings_csv> <location_framework_csv> <output_candidates_csv> [output_top_mappings_csv] [output_augmented_mappings_csv]\n";
		return 1;
	}

	try
	{
		const fs::path ck3_game_path = argv[1];
		const fs::path eu5_game_path = argv[2];
		const fs::path control_province_mappings_path = argv[3];
		const fs::path location_framework_path = argv[4];
		const fs::path output_candidates_path = argv[5];
		const fs::path output_mappings_path =
				argc >= 7 ? fs::path(argv[6]) : output_candidates_path.parent_path() / "map_correspondence_mappings.csv";
		const fs::path output_augmented_mappings_path =
				argc >= 8 ? fs::path(argv[7]) : output_candidates_path.parent_path() / "map_correspondence_augmented_mappings.csv";

		ck3eu5::diagnostics::DiagnosticsReport diagnostics;
		ck3eu5::maps::MapCorrespondenceBuilder builder;
		const auto result =
				builder.build(ck3_game_path, eu5_game_path, control_province_mappings_path, location_framework_path, diagnostics);
		builder.writeCandidatesCsv(result, output_candidates_path);
		builder.writeTopMappingsCsv(result, output_mappings_path);
		builder.writeAugmentedMappingsCsv(control_province_mappings_path, result, output_augmented_mappings_path);

		std::cout << diagnostics.summary();
		std::cout << "Wrote " << output_candidates_path.string() << ", " << output_mappings_path.string() << ", and " <<
				output_augmented_mappings_path.string() << ".\n";
		return diagnostics.hasErrors() ? 1 : 0;
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Error: " << exception.what() << '\n';
		return 1;
	}
}
