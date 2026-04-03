#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace ck3eu5::config {

struct Configuration
{
	std::filesystem::path config_path;
	std::filesystem::path ck3_input_path;
	std::filesystem::path ck3_game_path;
	std::filesystem::path output_mod_path;
	std::filesystem::path eu5_game_path;
	std::filesystem::path location_framework_path;
	std::filesystem::path province_mappings_path;
	std::filesystem::path title_mappings_path;
	std::filesystem::path culture_mappings_path;
	std::filesystem::path religion_mappings_path;
	std::filesystem::path government_mappings_path;
	std::filesystem::path country_colors_path;

	std::string preprocessor_command;

	std::string mod_name = "CK3 to EU5 Converted World";
	std::string mod_id = "ck3_to_eu5_generated";
	std::string mod_version = "0.1.0";
	std::string supported_game_version = "1.1.10";

	bool verbose_logging = false;
	bool write_debug_snapshots = true;
	bool minimal_government_setup = false;
	bool validation_force_monarchy = false;
	int validation_country_offset = 0;
	int validation_country_limit = 0;
	bool auto_normalize_raw_ck3 = true;
	bool prefer_subject_realms = true;
	int minimum_subject_counties = 2;
	int default_technology_level = 1;
	int default_gold = 100;

	[[nodiscard]] std::vector<std::string> validate() const;
};

}  // namespace ck3eu5::config
