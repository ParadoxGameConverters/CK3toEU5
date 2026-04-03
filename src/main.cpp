#include "ck3/world_importer.h"
#include "common/logger.h"
#include "config/configuration_loader.h"
#include "convert/world_converter.h"
#include "diagnostics/diagnostics_report.h"
#include "eu5/framework_builder.h"
#include "eu5/installed_definitions_loader.h"
#include "eu5/world_sanitizer.h"
#include "eu5/world_validator.h"
#include "mappers/mapper_bundle.h"
#include "output/eu5_outputter.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
	try
	{
		const fs::path config_path = argc >= 2 ? fs::path(argv[1]) : fs::path("examples/sample_config.cfg");

		ck3eu5::config::ConfigurationLoader configuration_loader;
		auto configuration = configuration_loader.load(config_path);
		ck3eu5::common::Logger::setVerbose(configuration.verbose_logging);

		ck3eu5::common::Logger::info("CK3ToEU5 starting.");
		ck3eu5::common::Logger::info("Using configuration: " + config_path.string());

		for (const auto& problem: configuration.validate())
		{
			throw std::runtime_error(problem);
		}

		ck3eu5::diagnostics::DiagnosticsReport diagnostics;

		ck3eu5::ck3::WorldImporter ck3_importer;
		auto ck3_world = ck3_importer.importFromConfiguration(configuration, diagnostics);

		ck3eu5::eu5::WorldFrameworkBuilder framework_builder;
		ck3eu5::common::Logger::info("Loading EU5 framework...");
		auto framework = framework_builder.load(configuration, diagnostics);

		ck3eu5::mappers::MapperBundleBuilder mapper_builder;
		ck3eu5::common::Logger::info("Loading mapping bundle...");
		auto mappers = mapper_builder.load(configuration, diagnostics);

		ck3eu5::convert::WorldConverter converter;
		ck3eu5::common::Logger::info("Converting world...");
		auto eu5_world = converter.convert(ck3_world, framework, mappers, configuration, diagnostics);

		if (!configuration.eu5_game_path.empty())
		{
			ck3eu5::common::Logger::info("Validating against installed EU5 definitions...");
			ck3eu5::eu5::InstalledDefinitionsLoader definitions_loader;
			const auto definitions = definitions_loader.load(configuration.eu5_game_path, diagnostics);
			ck3eu5::eu5::WorldSanitizer sanitizer;
			sanitizer.sanitize(eu5_world, framework, configuration, definitions, diagnostics);
			ck3eu5::eu5::WorldValidator validator;
			validator.validate(eu5_world, definitions, diagnostics);
		}

		ck3eu5::output::Eu5Outputter outputter;
		outputter.write(eu5_world, framework, configuration, diagnostics);

		ck3eu5::common::Logger::info("Done.");
		std::cout << '\n' << diagnostics.summary() << '\n';

		return diagnostics.hasErrors() ? 2 : 0;
	}
	catch (const std::exception& exception)
	{
		ck3eu5::common::Logger::error(exception.what());
		return 1;
	}
}
