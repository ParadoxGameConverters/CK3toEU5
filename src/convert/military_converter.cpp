#include "convert/military_converter.h"

#include <algorithm>
#include <cmath>

namespace ck3eu5::convert {
namespace {

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

void addForcePlan(eu5::World& world,
	 const std::string& key,
	 const std::string& tag,
	 const std::string& branch,
	 const std::string& home_location,
	 const std::string& stance,
	 const int levy_estimate,
	 const int standing_estimate)
{
	world.force_plans.push_back({.key = key,
		 .tag = tag,
		 .branch = branch,
		 .home_location = home_location,
		 .stance = stance,
		 .levy_estimate = levy_estimate,
		 .standing_estimate = standing_estimate});
}

void addUnit(std::vector<eu5::StartForceUnit>& units, const std::string& type, const int count)
{
	if (type.empty() || count <= 0)
	{
		return;
	}
	const auto existing = std::find_if(units.begin(), units.end(), [&type](const auto& unit) { return unit.type == type; });
	if (existing != units.end())
	{
		existing->count += count;
		return;
	}
	units.push_back({.type = type, .count = count});
}

void addStartForce(eu5::World& world,
	 const std::string& key,
	 const std::string& tag,
	 const std::string& branch,
	 const std::string& location,
	 const std::string& commander_character_key,
	 const bool wartime,
	 std::vector<eu5::StartForceUnit> units)
{
	if (location.empty() || units.empty())
	{
		return;
	}
	world.start_forces.push_back({.key = key,
		 .tag = tag,
		 .branch = branch,
		 .location = location,
		 .commander_character_key = commander_character_key,
		 .units = std::move(units),
		 .wartime = wartime});
}

std::string selectMilitaryStance(const eu5::Country& country, const CountryStats& stats, const BorderGraph& border_graph)
{
	const auto border_it = border_graph.country_neighbors.find(country.tag);
	const auto border_count = border_it == border_graph.country_neighbors.end() ? 0 : static_cast<int>(border_it->second.size());

	if (country.belligerent_vs_conciliatory >= 20 || border_count >= 3)
	{
		return "aggressive_military_stance";
	}
	if (country.offensive_vs_defensive >= 15)
	{
		return "defensive_military_stance";
	}
	if (country.land_vs_naval <= -20 && stats.coastal_locations > 0)
	{
		return "supportive_military_stance";
	}
	if (country.belligerent_vs_conciliatory <= -15)
	{
		return "passive_military_stance";
	}
	return "normal_military_stance";
}

std::string selectInfantryUnit(const eu5::Country& country)
{
	if (country.government_type == "steppe_horde" || country.government_type == "tribe")
	{
		return "a_warriors";
	}
	if (country.starting_technology_level >= 2)
	{
		return "a_men_at_arms";
	}
	return "a_footmen";
}

std::string selectRangedUnit(const eu5::Country& country)
{
	if (country.government_type == "steppe_horde" || country.government_type == "tribe")
	{
		return "a_tribesmen";
	}
	if (country.starting_technology_level >= 2)
	{
		return "a_crossbowmen";
	}
	return "a_archers";
}

std::string selectCavalryUnit(const eu5::Country& country)
{
	if (country.government_type == "steppe_horde" || country.government_type == "tribe")
	{
		return "a_horsemen";
	}
	if (country.starting_technology_level >= 3)
	{
		return "a_armored_horsemen";
	}
	return "a_horsemen";
}

std::string selectAuxiliaryUnit(const eu5::Country& country)
{
	if (country.government_type == "steppe_horde" || country.government_type == "tribe")
	{
		return "a_chieftains";
	}
	return "a_camp_followers";
}

std::string selectLightShip(const eu5::Country& country)
{
	if (country.starting_technology_level >= 5)
	{
		return "n_frigate";
	}
	if (country.starting_technology_level >= 4)
	{
		return "n_pinnace";
	}
	if (country.starting_technology_level >= 3)
	{
		return "n_caravel";
	}
	return "n_barque";
}

std::string selectGalley(const eu5::Country& country)
{
	if (country.starting_technology_level >= 5)
	{
		return "n_xebec";
	}
	if (country.starting_technology_level >= 4)
	{
		return "n_galleass";
	}
	if (country.starting_technology_level >= 3)
	{
		return "n_war_galley";
	}
	if (country.starting_technology_level >= 2)
	{
		return "n_mediterrannean_galley";
	}
	return "n_traditional_galley";
}

std::string selectTransport(const eu5::Country& country)
{
	if (country.starting_technology_level >= 5)
	{
		return "n_merchantman";
	}
	if (country.starting_technology_level >= 4)
	{
		return "n_brig";
	}
	if (country.starting_technology_level >= 3)
	{
		return "n_flute";
	}
	if (country.starting_technology_level >= 2)
	{
		return "n_hulk";
	}
	return "n_cog";
}

std::vector<eu5::StartForceUnit> buildArmyUnits(const eu5::Country& country, const int levy_estimate, const int standing_estimate)
{
	const int total_units = std::max(2, standing_estimate + std::max(1, levy_estimate / 2));
	const int cavalry_target =
			country.government_type == "steppe_horde" ? std::max(1, total_units / 2) :
			(country.land_vs_naval >= 10 ? std::max(1, total_units / 4) : total_units / 6);
	const int ranged_target = total_units >= 4 ? std::max(1, total_units / 4) : 0;
	const int auxiliary_target = total_units >= 6 ? 1 : 0;
	const int infantry_target = std::max(1, total_units - cavalry_target - ranged_target - auxiliary_target);

	std::vector<eu5::StartForceUnit> units;
	addUnit(units, selectInfantryUnit(country), infantry_target);
	addUnit(units, selectRangedUnit(country), ranged_target);
	addUnit(units, selectCavalryUnit(country), cavalry_target);
	addUnit(units, selectAuxiliaryUnit(country), auxiliary_target);
	return units;
}

std::vector<eu5::StartForceUnit> buildNavyUnits(const eu5::Country& country, const int levy_estimate, const int standing_estimate)
{
	const int total_ships = std::max(1, standing_estimate + std::max(1, levy_estimate / 2));
	const int transports = std::max(1, total_ships / 3);
	const int light_ships = std::max(1, total_ships / 2);
	const int galleys = std::max(0, total_ships - transports - light_ships);

	std::vector<eu5::StartForceUnit> units;
	addUnit(units, selectLightShip(country), light_ships);
	addUnit(units, selectTransport(country), transports);
	if (galleys > 0)
	{
		addUnit(units, selectGalley(country), galleys);
	}
	return units;
}

int estimateArmyLevy(const CountryStats& stats, const ck3::Character* source_character)
{
	int estimate = std::max(1, static_cast<int>(stats.total_population / 18.0 + stats.total_development * 0.35));
	if (source_character && source_character->realm_levy > 0.0)
	{
		estimate = std::max(estimate, static_cast<int>(std::lround(source_character->realm_levy / 450.0)));
	}
	return estimate;
}

int estimateArmyProfessionals(const eu5::Country& country, const CountryStats& stats, const ck3::Character* source_character)
{
	int estimate = std::max(0, static_cast<int>(stats.city_locations * 2 + stats.average_development / 5.0));
	if (source_character && source_character->realm_current_strength > 0.0)
	{
		const double levy_component = std::max(source_character->realm_levy, source_character->realm_current_strength * 0.65);
		const double professionals = std::max(0.0, source_character->realm_current_strength - levy_component);
		estimate = std::max(estimate, static_cast<int>(std::lround(professionals / 350.0)));
	}
	if (country.government_type == "steppe_horde")
	{
		estimate = std::max(estimate, 2);
	}
	return estimate;
}

int estimateNavyLevy(const CountryStats& stats, const ck3::Character* source_character)
{
	int estimate = std::max(1, static_cast<int>(stats.coastal_locations + stats.total_development / 25.0));
	if (source_character && source_character->realm_current_strength > 0.0 && stats.coastal_locations > 0)
	{
		estimate = std::max(estimate, static_cast<int>(std::lround((source_character->realm_current_strength / 2400.0) *
																									std::min<double>(1.0, stats.coastal_locations / 6.0))));
	}
	return estimate;
}

int estimateNavyProfessionals(const CountryStats& stats)
{
	return std::max(0, static_cast<int>(stats.coastal_locations / 2 + stats.city_locations / 2));
}

}  // namespace

void MilitaryConverter::convert(const ck3::World& ck3_world,
	 const BorderGraph& border_graph,
	 const eu5::WorldFramework& framework,
	 eu5::World& world) const
{
	world.force_plans.clear();
	world.start_forces.clear();

	for (auto& [tag, country]: world.countries)
	{
		const auto stats = measureCountryStats(country, world, framework);
		const auto stance = selectMilitaryStance(country, stats, border_graph);
		const auto* source_character = ck3_world.getCharacter(country.source_character_id);
		const auto border_locations_it = border_graph.border_locations.find(tag);
		const auto border_locations =
				border_locations_it == border_graph.border_locations.end() ? std::set<std::string>{} : border_locations_it->second;

		if (!country.capital_location.empty())
		{
			const auto capital_it = world.locations.find(country.capital_location);
			if (capital_it != world.locations.end())
			{
				if (capital_it->second.rank == "city" || capital_it->second.rank == "town")
				{
					addBuilding(world, country.government_type == "monarchy" ? "sergeantry" : "barracks", country.capital_location, tag, 1);
				}
				else
				{
					addBuilding(world, "training_fields", country.capital_location, tag, 1);
				}
				addBuilding(world,
					 stats.total_development >= 40.0 ? "castle" : "stockade",
					 country.capital_location,
					 tag,
					 1);

				const auto* capital_definition = framework.getLocation(country.capital_location);
				if (capital_definition && capital_definition->has_port)
				{
					addBuilding(world, "coastal_fort", country.capital_location, tag, 1);
					addBuilding(world, stats.total_development >= 70.0 ? "dry_dock" : "dock", country.capital_location, tag, 1);
				}
			}
		}

		int fortified_borders = 0;
		for (const auto& border_location: border_locations)
		{
			if (fortified_borders >= 3)
			{
				break;
			}
			const auto location_it = world.locations.find(border_location);
			if (location_it == world.locations.end())
			{
				continue;
			}
			addBuilding(world,
				 location_it->second.rank == "rural_settlement" ? "stockade" : "castle",
				 border_location,
				 tag,
				 1);
			if (const auto* definition = framework.getLocation(border_location); definition && definition->has_port)
			{
				addBuilding(world, "coastal_fort", border_location, tag, 1);
			}
			++fortified_borders;
		}

		int recruitment_centers = 0;
		for (const auto& location_key: country.owned_core_locations)
		{
			if (recruitment_centers >= 2)
			{
				break;
			}
			if (location_key == country.capital_location)
			{
				continue;
			}
			const auto location_it = world.locations.find(location_key);
			if (location_it == world.locations.end())
			{
				continue;
			}
			if (location_it->second.rank == "city" || location_it->second.rank == "town")
			{
				addBuilding(world, "barracks", location_key, tag, 1);
				++recruitment_centers;
			}
			else if (location_it->second.development >= 10.0)
			{
				addBuilding(world, "regimental_camp", location_key, tag, 1);
				++recruitment_centers;
			}
		}

		const int army_levies = estimateArmyLevy(stats, source_character);
		const int army_professionals = estimateArmyProfessionals(country, stats, source_character);
		addForcePlan(world,
			 "army_" + tag,
			 tag,
			 "army",
			 country.capital_location,
			 stance,
			 army_levies,
			 army_professionals);
		addStartForce(world,
			 "army_" + tag,
			 tag,
			 "army",
			 country.capital_location,
			 country.ruler_character_key,
			 false,
			 buildArmyUnits(country, army_levies, army_professionals));

		if (stats.coastal_locations > 0)
		{
			std::string naval_base = country.capital_location;
			for (const auto& market: world.markets)
			{
				if (market.owner_tag != tag)
				{
					continue;
				}
				if (const auto* definition = framework.getLocation(market.location); definition && definition->has_port)
				{
					naval_base = market.location;
					break;
				}
			}

			const int navy_levies = estimateNavyLevy(stats, source_character);
			const int navy_professionals = estimateNavyProfessionals(stats);
			addForcePlan(world,
				 "navy_" + tag,
				 tag,
				 "navy",
				 naval_base,
				 country.land_vs_naval <= -10 ? "aggressive_military_stance" : "normal_military_stance",
				 navy_levies,
				 navy_professionals);
			addStartForce(world,
				 "navy_" + tag,
				 tag,
				 "navy",
				 naval_base,
				 {},
				 false,
				 buildNavyUnits(country, navy_levies, navy_professionals));
		}
	}
}

}  // namespace ck3eu5::convert
