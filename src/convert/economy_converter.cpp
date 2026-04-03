#include "convert/economy_converter.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ck3eu5::convert {
namespace {

double marketScore(const eu5::LocationInstance& location, const eu5::LocationDefinition* definition, const bool capital)
{
	double score = location.development * 4.0;
	if (location.rank == "city")
	{
		score += 24.0;
	}
	else if (location.rank == "town")
	{
		score += 12.0;
	}
	if (definition && definition->coastal)
	{
		score += 12.0;
	}
	if (capital)
	{
		score += 18.0;
	}
	if (!location.town_setup.empty())
	{
		score += 4.0;
	}
	return score;
}

void addBuilding(eu5::World& world, const std::string& type, const std::string& location, const std::string& tag, const int level)
{
	const auto exists = std::any_of(world.buildings.begin(), world.buildings.end(), [&](const auto& building) {
		return building.type == type && building.location == location && building.tag == tag;
	});
	if (!exists)
	{
		world.buildings.push_back({.type = type, .location = location, .tag = tag, .level = level});
	}
}

}  // namespace

void EconomyConverter::convert(const BorderGraph& border_graph, const eu5::WorldFramework& framework, eu5::World& world) const
{
	world.markets.clear();

	for (auto& [tag, country]: world.countries)
	{
		const auto stats = measureCountryStats(country, world, framework);
		const bool subject = isSubjectCountry(tag, world);

		std::vector<std::pair<std::string, double>> candidates;
		for (const auto& location_key: country.owned_core_locations)
		{
			const auto location_it = world.locations.find(location_key);
			if (location_it == world.locations.end())
			{
				continue;
			}
			candidates.emplace_back(location_key,
				 marketScore(location_it->second, framework.getLocation(location_key), location_key == country.capital_location));
		}
		std::sort(candidates.begin(), candidates.end(), [](const auto& lhs, const auto& rhs) {
			return lhs.second > rhs.second;
		});

		int market_slots = subject ? 0 : 1;
		if (stats.total_development >= 45.0 && stats.location_count >= 6)
		{
			++market_slots;
		}
		if (stats.total_development >= 90.0 && stats.location_count >= 12)
		{
			++market_slots;
		}
		if (country.country_rank == "rank_empire")
		{
			++market_slots;
		}
		if (subject && (country.country_rank == "rank_kingdom" || stats.total_development >= 30.0))
		{
			market_slots = 1;
		}
		market_slots = std::min<int>(market_slots, static_cast<int>(candidates.size()));

		std::set<std::string> used_areas;
		for (const auto& [location_key, score]: candidates)
		{
			if (market_slots <= 0)
			{
				break;
			}
			const auto* definition = framework.getLocation(location_key);
			if (definition && !definition->area.empty() && used_areas.contains(definition->area) && market_slots == 1)
			{
				continue;
			}

			world.markets.push_back({.location = location_key, .owner_tag = tag, .score = score});
			if (definition && !definition->area.empty())
			{
				used_areas.insert(definition->area);
			}
			--market_slots;
		}

		for (const auto& market: world.markets)
		{
			if (market.owner_tag != tag)
			{
				continue;
			}
			const auto location_it = world.locations.find(market.location);
			if (location_it == world.locations.end())
			{
				continue;
			}
			const auto* definition = framework.getLocation(market.location);
			if (location_it->second.rank != "city")
			{
				addBuilding(world, "local_markets", market.location, tag, 1);
			}
			if (location_it->second.rank == "city")
			{
				addBuilding(world, stats.coastal_locations > 0 ? "merchants_guild" : "guild_hall", market.location, tag, 1);
			}
			if (definition && definition->has_port)
			{
				addBuilding(world, stats.total_development >= 60.0 ? "shipyard" : "dock", market.location, tag, 1);
				if (stats.total_development >= 80.0 || border_graph.border_locations.contains(tag))
				{
					addBuilding(world, "dry_dock", market.location, tag, 1);
				}
			}
		}
	}
}

}  // namespace ck3eu5::convert
