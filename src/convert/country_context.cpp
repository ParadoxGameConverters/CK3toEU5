#include "convert/country_context.h"

namespace ck3eu5::convert {

bool isSubjectCountry(const std::string& tag, const eu5::World& world)
{
	for (const auto& relation: world.subject_relations)
	{
		if (relation.subject_tag == tag)
		{
			return true;
		}
	}
	return false;
}

CountryStats measureCountryStats(const eu5::Country& country, const eu5::World& world, const eu5::WorldFramework& framework)
{
	CountryStats stats;
	stats.location_count = country.owned_core_locations.size();

	for (const auto& location_key: country.owned_core_locations)
	{
		const auto location_it = world.locations.find(location_key);
		if (location_it == world.locations.end())
		{
			continue;
		}
		const auto& location = location_it->second;
		stats.total_development += location.development;
		for (const auto& pop: location.pops)
		{
			stats.total_population += pop.size;
		}
		if (location.rank == "city")
		{
			++stats.city_locations;
		}
		else if (location.rank == "town")
		{
			++stats.town_locations;
		}
		if (const auto* definition = framework.getLocation(location_key); definition && definition->coastal)
		{
			++stats.coastal_locations;
		}
		if (location_key == country.capital_location)
		{
			stats.capital_development = location.development;
		}
	}

	if (stats.location_count > 0)
	{
		stats.average_development = stats.total_development / static_cast<double>(stats.location_count);
	}

	for (const auto& relation: world.subject_relations)
	{
		if (relation.liege_tag == country.tag)
		{
			++stats.subject_count;
		}
	}

	return stats;
}

BorderGraph buildBorderGraph(const ck3::World& ck3_world,
	 const std::unordered_map<std::string, std::string>& county_to_primary_location,
	 const std::unordered_map<std::string, std::string>& county_to_country_character,
	 const std::unordered_map<std::string, std::string>& character_to_tag)
{
	BorderGraph graph;

	for (const auto& [county_key, county]: ck3_world.counties)
	{
		const auto owner_it = county_to_country_character.find(county_key);
		const auto from_location_it = county_to_primary_location.find(county_key);
		if (owner_it == county_to_country_character.end() || from_location_it == county_to_primary_location.end())
		{
			continue;
		}
		const auto tag_it = character_to_tag.find(owner_it->second);
		if (tag_it == character_to_tag.end())
		{
			continue;
		}
		const auto& from_tag = tag_it->second;
		for (const auto& neighbor_key: county.neighbors)
		{
			const auto neighbor_owner_it = county_to_country_character.find(neighbor_key);
			if (neighbor_owner_it == county_to_country_character.end())
			{
				continue;
			}
			const auto neighbor_tag_it = character_to_tag.find(neighbor_owner_it->second);
			if (neighbor_tag_it == character_to_tag.end())
			{
				continue;
			}
			const auto& to_tag = neighbor_tag_it->second;
			if (from_tag == to_tag)
			{
				continue;
			}
			graph.country_neighbors[from_tag].insert(to_tag);
			graph.border_locations[from_tag].insert(from_location_it->second);
		}
	}

	return graph;
}

}  // namespace ck3eu5::convert
