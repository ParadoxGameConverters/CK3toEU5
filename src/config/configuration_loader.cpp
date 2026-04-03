#include "config/configuration_loader.h"

#include "common/filesystem_utils.h"
#include "common/string_utils.h"

#include <sstream>
#include <stdexcept>

namespace ck3eu5::config {
namespace fs = std::filesystem;

fs::path ConfigurationLoader::resolvePath(const fs::path& config_path, const std::string& candidate)
{
	if (candidate.empty())
	{
		return {};
	}
	const fs::path path(candidate);
	if (path.is_absolute())
	{
		return path;
	}
	return config_path.parent_path() / path;
}

Configuration ConfigurationLoader::load(const fs::path& path) const
{
	Configuration configuration;
	configuration.config_path = path;

	const auto content = common::readTextFile(path);
	std::istringstream input(content);
	std::string line;
	while (std::getline(input, line))
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		const auto trimmed = common::trim(line);
		if (trimmed.empty() || trimmed.starts_with('#'))
		{
			continue;
		}
		const auto delimiter = trimmed.find('=');
		if (delimiter == std::string::npos)
		{
			throw std::runtime_error("Invalid configuration line: " + trimmed);
		}
		const auto key = common::trim(trimmed.substr(0, delimiter));
		const auto value = common::stripQuotes(trimmed.substr(delimiter + 1));

		if (key == "ck3_input")
		{
			configuration.ck3_input_path = resolvePath(path, value);
		}
		else if (key == "ck3_game_path")
		{
			configuration.ck3_game_path = resolvePath(path, value);
		}
		else if (key == "eu5_game_path")
		{
			configuration.eu5_game_path = resolvePath(path, value);
		}
		else if (key == "output_mod_path")
		{
			configuration.output_mod_path = resolvePath(path, value);
		}
		else if (key == "location_framework")
		{
			configuration.location_framework_path = resolvePath(path, value);
		}
		else if (key == "province_mappings")
		{
			configuration.province_mappings_path = resolvePath(path, value);
		}
		else if (key == "title_mappings")
		{
			configuration.title_mappings_path = resolvePath(path, value);
		}
		else if (key == "culture_mappings")
		{
			configuration.culture_mappings_path = resolvePath(path, value);
		}
		else if (key == "religion_mappings")
		{
			configuration.religion_mappings_path = resolvePath(path, value);
		}
		else if (key == "government_mappings")
		{
			configuration.government_mappings_path = resolvePath(path, value);
		}
		else if (key == "country_colors")
		{
			configuration.country_colors_path = resolvePath(path, value);
		}
		else if (key == "preprocessor_command")
		{
			configuration.preprocessor_command = value;
		}
		else if (key == "mod_name")
		{
			configuration.mod_name = value;
		}
		else if (key == "mod_id")
		{
			configuration.mod_id = value;
		}
		else if (key == "mod_version")
		{
			configuration.mod_version = value;
		}
		else if (key == "supported_game_version")
		{
			configuration.supported_game_version = value;
		}
		else if (key == "verbose_logging")
		{
			configuration.verbose_logging = common::parseBool(value, false);
		}
		else if (key == "write_debug_snapshots")
		{
			configuration.write_debug_snapshots = common::parseBool(value, true);
		}
		else if (key == "minimal_government_setup")
		{
			configuration.minimal_government_setup = common::parseBool(value, false);
		}
		else if (key == "validation_force_monarchy")
		{
			configuration.validation_force_monarchy = common::parseBool(value, false);
		}
		else if (key == "validation_country_offset")
		{
			configuration.validation_country_offset = common::parseInt(value).value_or(0);
		}
		else if (key == "validation_country_limit")
		{
			configuration.validation_country_limit = common::parseInt(value).value_or(0);
		}
		else if (key == "auto_normalize_raw_ck3")
		{
			configuration.auto_normalize_raw_ck3 = common::parseBool(value, true);
		}
		else if (key == "prefer_subject_realms")
		{
			configuration.prefer_subject_realms = common::parseBool(value, true);
		}
		else if (key == "minimum_subject_counties")
		{
			configuration.minimum_subject_counties = common::parseInt(value).value_or(2);
		}
		else if (key == "default_technology_level")
		{
			configuration.default_technology_level = common::parseInt(value).value_or(1);
		}
		else if (key == "default_gold")
		{
			configuration.default_gold = common::parseInt(value).value_or(100);
		}
	}

	return configuration;
}

}  // namespace ck3eu5::config
