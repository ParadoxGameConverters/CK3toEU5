#pragma once

#include "ck3/world.h"
#include "eu5/framework.h"
#include "eu5/world.h"

#include <map>
#include <set>
#include <string>
#include <unordered_map>

namespace ck3eu5::convert {

struct CountryStats
{
	size_t location_count = 0;
	size_t city_locations = 0;
	size_t town_locations = 0;
	size_t coastal_locations = 0;
	size_t subject_count = 0;
	double total_development = 0.0;
	double average_development = 0.0;
	double capital_development = 0.0;
	double total_population = 0.0;
};

struct BorderGraph
{
	std::map<std::string, std::set<std::string>> country_neighbors;
	std::map<std::string, std::set<std::string>> border_locations;
};

[[nodiscard]] bool isSubjectCountry(const std::string& tag, const eu5::World& world);
[[nodiscard]] CountryStats measureCountryStats(const eu5::Country& country,
	 const eu5::World& world,
	 const eu5::WorldFramework& framework);
[[nodiscard]] BorderGraph buildBorderGraph(const ck3::World& ck3_world,
	 const std::unordered_map<std::string, std::string>& county_to_primary_location,
	 const std::unordered_map<std::string, std::string>& county_to_country_character,
	 const std::unordered_map<std::string, std::string>& character_to_tag);

}  // namespace ck3eu5::convert
