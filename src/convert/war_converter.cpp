#include "convert/war_converter.h"

#include "convert/country_context.h"
#include "common/string_utils.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace ck3eu5::convert {
namespace {

bool isSubjectOf(const std::string& tag, const std::string& liege_tag, const eu5::World& world)
{
	return std::any_of(world.subject_relations.begin(), world.subject_relations.end(), [&](const auto& relation) {
		return relation.subject_tag == tag && relation.liege_tag == liege_tag;
	});
}

bool hasAllianceWith(const std::string& first, const std::string& second, const eu5::World& world)
{
	return std::any_of(world.scripted_relations.begin(), world.scripted_relations.end(), [&](const auto& relation) {
		if (relation.type != "alliance")
		{
			return false;
		}
		if (relation.mutual)
		{
			return (relation.first_tag == first && relation.second_tag == second) ||
					 (relation.first_tag == second && relation.second_tag == first);
		}
		return relation.first_tag == first && relation.second_tag == second;
	});
}

std::string resolveCountryTag(const ck3::World& ck3_world,
	 const std::set<std::string>& country_characters,
	 const std::unordered_map<std::string, std::string>& character_to_tag,
	 const std::string& character_id)
{
	std::string current = character_id;
	while (!current.empty())
	{
		if (const auto tag_it = character_to_tag.find(current); tag_it != character_to_tag.end())
		{
			return tag_it->second;
		}
		if (!country_characters.contains(current))
		{
			const auto* character = ck3_world.getCharacter(current);
			if (!character)
			{
				break;
			}
			current = character->liege;
			continue;
		}
		break;
	}
	return {};
}

bool isCivilWar(const ck3::World& ck3_world, const ck3::War& war)
{
	const auto cb_type = common::toLower(war.cb_type);
	if (cb_type.find("civil") != std::string::npos || cb_type.find("claimant") != std::string::npos ||
		 cb_type.find("peasant") != std::string::npos || cb_type.find("dissolution") != std::string::npos ||
		 cb_type.find("independence") != std::string::npos || cb_type.find("liberty") != std::string::npos ||
		 cb_type.find("tyranny") != std::string::npos || cb_type.find("depose") != std::string::npos ||
		 cb_type.find("faction") != std::string::npos)
	{
		return true;
	}
	if (war.attacker_id.empty() || war.defender_id.empty())
	{
		return false;
	}
	const auto attacker_top_liege = ck3_world.topLiegeOfCharacter(war.attacker_id);
	const auto defender_top_liege = ck3_world.topLiegeOfCharacter(war.defender_id);
	return !attacker_top_liege.empty() && attacker_top_liege == defender_top_liege;
}

std::string resolveWarTargetLocation(const ck3::World& ck3_world,
	 const ck3::War& war,
	 const std::unordered_map<std::string, std::string>& county_to_primary_location)
{
	for (const auto& targeted_title: war.targeted_titles)
	{
		const auto* title = ck3_world.getTitle(targeted_title);
		if (!title)
		{
			continue;
		}
		if (title->rank == ck3::TitleRank::County)
		{
			if (const auto county_it = county_to_primary_location.find(title->key); county_it != county_to_primary_location.end())
			{
				return county_it->second;
			}
		}
		if (!title->capital_county.empty())
		{
			if (const auto county_it = county_to_primary_location.find(title->capital_county); county_it != county_to_primary_location.end())
			{
				return county_it->second;
			}
		}
		for (const auto& county_key: title->owned_de_facto_counties)
		{
			if (const auto county_it = county_to_primary_location.find(county_key); county_it != county_to_primary_location.end())
			{
				return county_it->second;
			}
		}
		for (const auto& county_key: title->owned_de_jure_counties)
		{
			if (const auto county_it = county_to_primary_location.find(county_key); county_it != county_to_primary_location.end())
			{
				return county_it->second;
			}
		}
	}
	return {};
}

std::vector<eu5::WarParticipant> convertParticipants(const ck3::World& ck3_world,
	 const std::set<std::string>& country_characters,
	 const std::unordered_map<std::string, std::string>& character_to_tag,
	 const std::vector<ck3::WarParticipant>& participants,
	 const std::string& leader_tag,
	 const eu5::World& world,
	 const bool attacker_side)
{
	std::vector<eu5::WarParticipant> converted;
	std::set<std::string> seen_tags;

	for (const auto& participant: participants)
	{
		const auto participant_tag = resolveCountryTag(ck3_world, country_characters, character_to_tag, participant.character_id);
		if (participant_tag.empty() || !seen_tags.insert(participant_tag).second)
		{
			continue;
		}

		eu5::WarParticipant converted_participant;
		converted_participant.tag = participant_tag;
		if (participant_tag == leader_tag && converted.empty())
		{
			converted_participant.reason = attacker_side ? "Instigator" : "Target";
			converted_participant.which.clear();
		}
		else if (isSubjectOf(participant_tag, leader_tag, world))
		{
			converted_participant.caller_tag = leader_tag;
			converted_participant.reason = "Subject";
			converted_participant.which.clear();
		}
		else
		{
			converted_participant.caller_tag = leader_tag;
			converted_participant.reason = "Scripted";
			converted_participant.which = hasAllianceWith(participant_tag, leader_tag, world) ? "alliance" : "call_to_war";
		}
		converted.push_back(std::move(converted_participant));
	}

	if (converted.empty() && !leader_tag.empty())
	{
		eu5::WarParticipant participant;
		participant.tag = leader_tag;
		participant.reason = attacker_side ? "Instigator" : "Target";
		converted.push_back(std::move(participant));
	}

	return converted;
}

const eu5::StartForce* findBaseForce(const eu5::World& world, const std::string& tag, const std::string& branch)
{
	const auto it = std::find_if(world.start_forces.begin(), world.start_forces.end(), [&](const auto& force) {
		return force.tag == tag && force.branch == branch && !force.wartime;
	});
	return it == world.start_forces.end() ? nullptr : &*it;
}

std::vector<eu5::StartForceUnit> scaleUnits(const std::vector<eu5::StartForceUnit>& units, const double factor)
{
	std::vector<eu5::StartForceUnit> scaled;
	for (const auto& unit: units)
	{
		const int count = std::max(1, static_cast<int>(std::lround(unit.count * factor)));
		scaled.push_back({.type = unit.type, .count = count});
	}
	return scaled;
}

void addWartimeForce(eu5::World& world,
	 const std::string& war_key,
	 const std::string& tag,
	 const std::string& branch,
	 const std::string& location,
	 const std::string& commander_character_key)
{
	const auto* base_force = findBaseForce(world, tag, branch);
	if (!base_force || location.empty())
	{
		return;
	}
	const auto key = war_key + "_" + branch + "_" + common::sanitizeIdentifier(tag);
	const auto exists = std::any_of(world.start_forces.begin(), world.start_forces.end(), [&](const auto& force) {
		return force.key == key;
	});
	if (exists)
	{
		return;
	}
	world.start_forces.push_back({.key = key,
		 .tag = tag,
		 .branch = branch,
		 .location = location,
		 .commander_character_key = commander_character_key,
		 .units = scaleUnits(base_force->units, branch == "navy" ? 0.5 : 0.6),
		 .wartime = true});
}

}  // namespace

void WarConverter::convert(const ck3::World& ck3_world,
	 const std::set<std::string>& country_characters,
	 const std::unordered_map<std::string, std::string>& character_to_tag,
	 const std::unordered_map<std::string, std::string>& county_to_primary_location,
	 eu5::World& world) const
{
	world.wars.clear();
	std::map<std::string, int> ordinal_by_name;

	for (const auto& [war_id, ck3_war]: ck3_world.wars)
	{
		const auto leader_attacker_tag = resolveCountryTag(ck3_world, country_characters, character_to_tag, ck3_war.attacker_id);
		const auto leader_defender_tag = resolveCountryTag(ck3_world, country_characters, character_to_tag, ck3_war.defender_id);
		if (leader_attacker_tag.empty() || leader_defender_tag.empty() || leader_attacker_tag == leader_defender_tag)
		{
			continue;
		}

		eu5::War war;
		war.key = "war_" + common::sanitizeIdentifier(war_id);
		war.civil_war = isCivilWar(ck3_world, ck3_war);
		war.name = war.civil_war ? "CIVIL_WAR_NAME" :
							 (common::toLower(ck3_war.cb_type).find("holy") != std::string::npos ||
									  common::toLower(ck3_war.cb_type).find("crusade") != std::string::npos ? "CRUSADE_WAR_NAME" :
																								 "NORMAL_WAR_NAME");
		war.ordinal = ++ordinal_by_name[war.name];
		war.first_tag = leader_attacker_tag;
		war.second_tag = leader_defender_tag;
		war.start_date = ck3_war.start_date.empty() ? world.date : ck3_war.start_date;
		war.action_date = world.date;
		war.target_location = resolveWarTargetLocation(ck3_world, ck3_war, county_to_primary_location);
		war.attackers = convertParticipants(
				ck3_world, country_characters, character_to_tag, ck3_war.attackers, leader_attacker_tag, world, true);
		war.defenders = convertParticipants(
				ck3_world, country_characters, character_to_tag, ck3_war.defenders, leader_defender_tag, world, false);
		if (war.attackers.empty() || war.defenders.empty())
		{
			continue;
		}
		world.wars.push_back(war);

		for (const auto& participant: world.wars.back().attackers)
		{
			const auto country_it = world.countries.find(participant.tag);
			if (country_it == world.countries.end())
			{
				continue;
			}
			const auto target_location =
				 !world.wars.back().target_location.empty() &&
								 world.locations.contains(world.wars.back().target_location) &&
								 world.locations.at(world.wars.back().target_location).owner_tag == participant.tag ?
						 world.wars.back().target_location :
						 country_it->second.capital_location;
			addWartimeForce(world, world.wars.back().key, participant.tag, "army", target_location, country_it->second.ruler_character_key);
			if (!world.wars.back().target_location.empty() && world.locations.contains(world.wars.back().target_location) &&
				 world.locations.at(world.wars.back().target_location).owner_tag == participant.tag &&
				 !country_it->second.capital_location.empty())
			{
				addWartimeForce(world, world.wars.back().key, participant.tag, "navy", target_location, {});
			}
		}

		for (const auto& participant: world.wars.back().defenders)
		{
			const auto country_it = world.countries.find(participant.tag);
			if (country_it == world.countries.end())
			{
				continue;
			}
			const auto target_location =
				 !world.wars.back().target_location.empty() &&
								 world.locations.contains(world.wars.back().target_location) &&
								 world.locations.at(world.wars.back().target_location).owner_tag == participant.tag ?
						 world.wars.back().target_location :
						 country_it->second.capital_location;
			addWartimeForce(world, world.wars.back().key, participant.tag, "army", target_location, country_it->second.ruler_character_key);
			if (!world.wars.back().target_location.empty() && world.locations.contains(world.wars.back().target_location) &&
				 world.locations.at(world.wars.back().target_location).owner_tag == participant.tag)
			{
				addWartimeForce(world, world.wars.back().key, participant.tag, "navy", target_location, {});
			}
		}
	}
}

}  // namespace ck3eu5::convert
