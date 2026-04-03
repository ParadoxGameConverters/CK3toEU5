#include "common/logger.h"
#include "diagnostics/diagnostics_report.h"
#include "eu5/installed_data_extractor.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
	try
	{
		if (argc < 3)
		{
			std::cerr << "Usage: ck3_to_eu5_extract_eu5_framework <eu5_game_path> <output_directory>\n";
			return 1;
		}

		const fs::path eu5_game_path = argv[1];
		const fs::path output_directory = argv[2];
		const fs::path location_framework_path = output_directory / "location_framework.csv";
		const fs::path country_colors_path = output_directory / "country_colors.csv";

		ck3eu5::common::Logger::setVerbose(true);
		ck3eu5::common::Logger::info("Extracting EU5 framework from: " + eu5_game_path.string());

		ck3eu5::diagnostics::DiagnosticsReport diagnostics;
		ck3eu5::eu5::InstalledDataExtractor extractor;
		const auto framework = extractor.extract(eu5_game_path, diagnostics);
		extractor.writeCsvs(framework, location_framework_path, country_colors_path);

		ck3eu5::common::Logger::info("Wrote: " + location_framework_path.string());
		ck3eu5::common::Logger::info("Wrote: " + country_colors_path.string());
		std::cout << diagnostics.summary() << '\n';
		return diagnostics.hasErrors() ? 2 : 0;
	}
	catch (const std::exception& exception)
	{
		ck3eu5::common::Logger::error(exception.what());
		return 1;
	}
}
