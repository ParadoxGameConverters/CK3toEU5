#pragma once

#include <map>
#include <string>
#include <vector>

namespace ck3eu5::eu5 {

struct LocationDefinition
{
	std::string key;
	std::string province_definition;
	std::string display_name;
	std::string raw_good;
	std::string region;
	std::string area;
	std::string climate;
	std::string topography;
	std::string default_rank = "rural_settlement";
	std::string town_setup;
	bool coastal = false;
	bool has_port = false;
};

struct CountryColorDefinition
{
	std::string tag;
	std::string color = "rgb { 120 120 120 }";
	std::string color2 = "rgb { 180 180 180 }";
	std::string color3 = "rgb { 90 90 90 }";
	std::string unit_color0 = "rgb { 120 120 120 }";
	std::string unit_color1 = "rgb { 180 180 180 }";
	std::string unit_color2 = "rgb { 90 90 90 }";
	std::string description_category = "administrative";
	int difficulty = 3;
};

struct WorldFramework
{
	std::map<std::string, LocationDefinition> locations;
	std::map<std::string, std::vector<std::string>> province_to_locations;
	std::map<std::string, CountryColorDefinition> colors;

	[[nodiscard]] const LocationDefinition* getLocation(const std::string& key) const;
	[[nodiscard]] std::vector<std::string> locationsForProvince(const std::string& province_definition) const;
};

}  // namespace ck3eu5::eu5
