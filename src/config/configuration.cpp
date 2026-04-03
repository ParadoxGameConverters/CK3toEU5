#include "config/configuration.h"

namespace ck3eu5::config {

std::vector<std::string> Configuration::validate() const
{
	std::vector<std::string> problems;
	if (ck3_input_path.empty())
	{
		problems.push_back("Missing ck3_input path");
	}
	if (output_mod_path.empty())
	{
		problems.push_back("Missing output_mod_path");
	}
	if (location_framework_path.empty())
	{
		problems.push_back("Missing location_framework path");
	}
	if (province_mappings_path.empty())
	{
		problems.push_back("Missing province_mappings path");
	}
	if (title_mappings_path.empty())
	{
		problems.push_back("Missing title_mappings path");
	}
	if (culture_mappings_path.empty())
	{
		problems.push_back("Missing culture_mappings path");
	}
	if (religion_mappings_path.empty())
	{
		problems.push_back("Missing religion_mappings path");
	}
	if (government_mappings_path.empty())
	{
		problems.push_back("Missing government_mappings path");
	}
	if (country_colors_path.empty())
	{
		problems.push_back("Missing country_colors path");
	}
	if (mod_name.empty())
	{
		problems.push_back("Missing mod_name");
	}
	if (mod_id.empty())
	{
		problems.push_back("Missing mod_id");
	}
	if (validation_country_offset < 0)
	{
		problems.push_back("validation_country_offset must be >= 0");
	}
	if (validation_country_limit < 0)
	{
		problems.push_back("validation_country_limit must be >= 0");
	}
	return problems;
}

}  // namespace ck3eu5::config
