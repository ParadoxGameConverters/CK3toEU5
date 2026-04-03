#include "ck3/world_importer.h"
#include "common/filesystem_utils.h"
#include "common/string_utils.h"
#include "config/configuration_loader.h"
#include "diagnostics/diagnostics_report.h"
#include "eu5/framework_builder.h"
#include "mappers/mapper_bundle.h"
#include "mappers/province_matcher.h"

#include <filesystem>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace {

std::string csvEscape(const std::string& value)
{
	std::string escaped = "\"";
	for (const char character: value)
	{
		if (character == '"')
		{
			escaped += "\"\"";
		}
		else
		{
			escaped.push_back(character);
		}
	}
	escaped.push_back('"');
	return escaped;
}

}  // namespace

int main(int argc, char** argv)
{
	if (argc < 2 || argc > 3)
	{
		std::cerr << "Usage: ck3_to_eu5_measure_mapping_coverage <config_path> [unmapped_report_csv]\n";
		return 1;
	}

	try
	{
		const fs::path config_path = argv[1];
		const fs::path unmapped_report_path = argc >= 3 ? fs::path(argv[2]) : fs::path();

		ck3eu5::config::ConfigurationLoader configuration_loader;
		const auto configuration = configuration_loader.load(config_path);
		for (const auto& problem: configuration.validate())
		{
			throw std::runtime_error(problem);
		}

		ck3eu5::diagnostics::DiagnosticsReport diagnostics;

		ck3eu5::ck3::WorldImporter importer;
		const auto world = importer.importFromConfiguration(configuration, diagnostics);

		ck3eu5::eu5::WorldFrameworkBuilder framework_builder;
		const auto framework = framework_builder.load(configuration, diagnostics);

		ck3eu5::mappers::MapperBundleBuilder mapper_builder;
		const auto mappers = mapper_builder.load(configuration, diagnostics);

		ck3eu5::mappers::ProvinceMatcher matcher(framework);

		size_t csv_mapped = 0;
		size_t auto_mapped = 0;
		size_t unmapped = 0;
		std::map<std::string, size_t> auto_source_counts;
		std::ostringstream unmapped_report;

		if (!unmapped_report_path.empty())
		{
			unmapped_report << "ck3_county,display_name,province_key,culture,faith,government,development,holdings,barony_keys,barony_display_names\n";
		}

		for (const auto& [county_key, county]: world.counties)
		{
			const auto mapped_locations = mappers.mapCountyToLocations(county_key);
			if (!mapped_locations.empty())
			{
				++csv_mapped;
				continue;
			}

			const auto auto_match = matcher.match(county);
			if (auto_match.has_value())
			{
				++auto_mapped;
				++auto_source_counts[auto_match->source];
				continue;
			}

			++unmapped;
			if (!unmapped_report_path.empty())
			{
				unmapped_report << csvEscape(county.key) << ',';
				unmapped_report << csvEscape(county.display_name) << ',';
				unmapped_report << csvEscape(county.province_key) << ',';
				unmapped_report << csvEscape(county.culture) << ',';
				unmapped_report << csvEscape(county.faith) << ',';
				unmapped_report << csvEscape(county.government) << ',';
				unmapped_report << county.development << ',';
				unmapped_report << csvEscape(std::to_string(county.holdings.size())) << ',';
				unmapped_report << csvEscape(ck3eu5::common::join(county.barony_keys, "|")) << ',';
				unmapped_report << csvEscape(ck3eu5::common::join(county.barony_display_names, "|")) << '\n';
			}
		}

		if (!unmapped_report_path.empty())
		{
			ck3eu5::common::writeTextFile(unmapped_report_path, unmapped_report.str(), ck3eu5::common::TextEncoding::Utf8NoBom);
		}

		std::cout << "Total counties: " << world.counties.size() << '\n';
		std::cout << "Mapped via CSV: " << csv_mapped << '\n';
		std::cout << "Mapped via auto-matcher: " << auto_mapped << '\n';
		for (const auto& [source, count]: auto_source_counts)
		{
			std::cout << "  " << source << ": " << count << '\n';
		}
		std::cout << "Unmapped: " << unmapped << '\n';
		if (!unmapped_report_path.empty())
		{
			std::cout << "Unmapped report: " << unmapped_report_path.string() << '\n';
		}
		std::cout << '\n' << diagnostics.summary();
		return diagnostics.hasErrors() ? 2 : 0;
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Error: " << exception.what() << '\n';
		return 1;
	}
}
