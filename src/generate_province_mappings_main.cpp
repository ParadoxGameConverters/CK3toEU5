#include "ck3/installed_titles.h"
#include "config/configuration.h"
#include "diagnostics/diagnostics_report.h"
#include "eu5/framework_builder.h"
#include "mappers/bootstrap_province_mapping_generator.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
	if (argc < 5)
	{
		std::cerr << "Usage: ck3_to_eu5_generate_province_mappings <ck3_game_path> <location_framework_csv> <existing_province_mappings_csv> <output_province_mappings_csv> [report_csv] [--ck3-mod <mod_descriptor_or_root> ...]\n";
		return 1;
	}

	try
	{
		const fs::path ck3_game_path = argv[1];
		const fs::path location_framework_path = argv[2];
		const fs::path existing_province_mappings_path = argv[3];
		const fs::path output_province_mappings_path = argv[4];
		int argument_index = 5;
		fs::path report_path = output_province_mappings_path.parent_path() / "province_mappings_report.csv";
		if (argument_index < argc && std::string_view(argv[argument_index]) != "--ck3-mod")
		{
			report_path = fs::path(argv[argument_index]);
			++argument_index;
		}

		std::vector<fs::path> mod_paths;
		while (argument_index < argc)
		{
			const std::string_view argument = argv[argument_index];
			if (argument != "--ck3-mod" || argument_index + 1 >= argc)
			{
				throw std::runtime_error("Invalid generator arguments. Use --ck3-mod <path> for each CK3 mod.");
			}
			mod_paths.emplace_back(argv[argument_index + 1]);
			argument_index += 2;
		}

		ck3eu5::diagnostics::DiagnosticsReport diagnostics;

		ck3eu5::ck3::InstalledTitlesLoader titles_loader;
		const auto installed_titles = titles_loader.load(ck3_game_path, mod_paths);

		ck3eu5::config::Configuration configuration;
		configuration.location_framework_path = location_framework_path;
		configuration.country_colors_path = location_framework_path.parent_path() / "country_colors.csv";

		ck3eu5::eu5::WorldFrameworkBuilder framework_builder;
		const auto framework = framework_builder.load(configuration, diagnostics);

		ck3eu5::mappers::BootstrapProvinceMappingGenerator generator;
		const auto result =
				generator.generate(installed_titles, framework, existing_province_mappings_path, diagnostics);
		generator.writeCsvs(result, output_province_mappings_path, report_path);

		std::cout << diagnostics.summary();
		std::cout << "Wrote " << output_province_mappings_path.string() << " and " << report_path.string() << ".\n";
		return diagnostics.hasErrors() ? 1 : 0;
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Error: " << exception.what() << '\n';
		return 1;
	}
}
