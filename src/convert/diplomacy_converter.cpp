#include "convert/diplomacy_converter.h"

#include <algorithm>
#include <map>
#include <tuple>

namespace ck3eu5::convert {
namespace {

int countryRankLevel(const std::string& rank)
{
	if (rank == "rank_empire")
	{
		return 4;
	}
	if (rank == "rank_kingdom")
	{
		return 3;
	}
	if (rank == "rank_duchy")
	{
		return 2;
	}
	return 1;
}

std::string determineSubjectType(const eu5::Country& liege, const eu5::Country& subject)
{
	if (subject.government_type == "tribe" && liege.country_rank != "rank_duchy")
	{
		return "tributary";
	}
	if (liege.country_rank == "rank_empire" && subject.country_rank == "rank_kingdom")
	{
		return "dominion";
	}
	if (liege.government_type == "monarchy" && subject.government_type == "monarchy")
	{
		return "fiefdom";
	}
	return "vassal";
}

std::string determineSubjectMilitaryStance(const eu5::Country& liege, const eu5::Country& subject)
{
	if (liege.belligerent_vs_conciliatory >= 25)
	{
		return "aggressive_military_stance";
	}
	if (liege.offensive_vs_defensive >= 20)
	{
		return "defensive_military_stance";
	}
	if (subject.government_type == "tribe" || subject.government_type == "theocracy")
	{
		return "supportive_military_stance";
	}
	if (liege.belligerent_vs_conciliatory <= -15)
	{
		return "passive_military_stance";
	}
	return "normal_military_stance";
}

int maxDiplomaticRelations(const eu5::Country& country)
{
	int cap = 3;
	if (country.country_rank == "rank_kingdom")
	{
		cap = 5;
	}
	else if (country.country_rank == "rank_empire")
	{
		cap = 7;
	}

	cap += std::min<int>(3, static_cast<int>(country.owned_core_locations.size() / 8));
	return cap;
}

int maxAllianceCount(const eu5::Country& country)
{
	if (country.country_rank == "rank_empire")
	{
		return country.owned_core_locations.size() >= 12 ? 3 : 2;
	}
	if (country.country_rank == "rank_kingdom")
	{
		return country.owned_core_locations.size() >= 10 ? 2 : 1;
	}
	return 0;
}

int maxGuaranteeCount(const eu5::Country& country)
{
	if (country.country_rank == "rank_empire")
	{
		return country.owned_core_locations.size() >= 14 ? 2 : 1;
	}
	if (country.country_rank == "rank_kingdom" && country.owned_core_locations.size() >= 12)
	{
		return 1;
	}
	return 0;
}

void addOpinion(eu5::World& world, const std::string& first, const std::string& second, const std::string& type)
{
	if (first.empty() || second.empty() || first == second || type.empty())
	{
		return;
	}
	const auto exists = std::any_of(world.opinions.begin(), world.opinions.end(), [&](const auto& opinion) {
		return opinion.first_tag == first && opinion.second_tag == second && opinion.type == type;
	});
	if (!exists)
	{
		world.opinions.push_back({.first_tag = first, .second_tag = second, .type = type});
	}
}

void addRival(eu5::World& world, const std::string& first, const std::string& second)
{
	if (first.empty() || second.empty() || first == second)
	{
		return;
	}
	const auto exists = std::any_of(world.rivals.begin(), world.rivals.end(), [&](const auto& rival) {
		return rival.first_tag == first && rival.second_tag == second;
	});
	if (!exists)
	{
		world.rivals.push_back({.first_tag = first, .second_tag = second});
	}
}

void addScriptedRelation(eu5::World& world,
	 const std::string& first,
	 const std::string& second,
	 const std::string& type,
	 const bool mutual)
{
	if (first.empty() || second.empty() || first == second || type.empty())
	{
		return;
	}
	const auto exists = std::any_of(world.scripted_relations.begin(), world.scripted_relations.end(), [&](const auto& relation) {
		return relation.first_tag == first && relation.second_tag == second && relation.type == type && relation.mutual == mutual;
	});
	if (!exists)
	{
		world.scripted_relations.push_back({.first_tag = first, .second_tag = second, .type = type, .mutual = mutual});
	}
}

bool hasCrossRealmDynasticTie(const ck3::World& ck3_world, const eu5::Country& first, const eu5::Country& second)
{
	const auto* first_character = ck3_world.getCharacter(first.source_character_id);
	const auto* second_character = ck3_world.getCharacter(second.source_character_id);
	if (!first_character || !second_character)
	{
		return false;
	}
	if (!first_character->dynasty_house_id.empty() && first_character->dynasty_house_id == second_character->dynasty_house_id)
	{
		return true;
	}
	if (first_character->spouse_id == second_character->id || second_character->spouse_id == first_character->id)
	{
		return true;
	}
	return false;
}

bool isIndependent(const std::string& tag, const eu5::World& world)
{
	return !isSubjectCountry(tag, world);
}

}  // namespace

void DiplomacyConverter::convert(const ck3::World& ck3_world,
	 const BorderGraph& border_graph,
	 const std::set<std::string>& country_characters,
	 const std::unordered_map<std::string, std::string>& character_to_tag,
	 eu5::World& world) const
{
	world.subject_relations.clear();
	world.scripted_relations.clear();
	world.opinions.clear();
	world.rivals.clear();

	for (const auto& character_id: country_characters)
	{
		const auto* character = ck3_world.getCharacter(character_id);
		if (!character || character->liege.empty())
		{
			continue;
		}
		std::string parent = character->liege;
		while (!parent.empty() && !country_characters.contains(parent))
		{
			const auto* liege = ck3_world.getCharacter(parent);
			if (!liege)
			{
				parent.clear();
				break;
			}
			parent = liege->liege;
		}
		if (parent.empty())
		{
			continue;
		}
		const auto subject_tag_it = character_to_tag.find(character_id);
		const auto liege_tag_it = character_to_tag.find(parent);
		if (subject_tag_it == character_to_tag.end() || liege_tag_it == character_to_tag.end())
		{
			continue;
		}
		if (subject_tag_it->second == liege_tag_it->second)
		{
			continue;
		}
		const auto& liege = world.countries.at(liege_tag_it->second);
		const auto& subject = world.countries.at(subject_tag_it->second);
		world.subject_relations.push_back({.liege_tag = liege.tag,
			 .subject_tag = subject.tag,
			 .subject_type = determineSubjectType(liege, subject),
			 .subject_military_stance = determineSubjectMilitaryStance(liege, subject),
			 .start_date = world.date});
		addOpinion(world, subject.tag, liege.tag, "opinion_supportive_monarch");
	}

	std::map<std::string, int> relation_load;
	for (const auto& relation: world.subject_relations)
	{
		++relation_load[relation.liege_tag];
		++relation_load[relation.subject_tag];
	}
	std::map<std::string, int> alliance_counts;
	std::map<std::string, int> guarantee_counts;

	for (const auto& [tag, neighbors]: border_graph.country_neighbors)
	{
		const auto country_it = world.countries.find(tag);
		if (country_it == world.countries.end())
		{
			continue;
		}
		const auto& country = country_it->second;
		if (!isIndependent(tag, world))
		{
			continue;
		}

		for (const auto& neighbor_tag: neighbors)
		{
			const auto neighbor_it = world.countries.find(neighbor_tag);
			if (neighbor_it == world.countries.end())
			{
				continue;
			}
			const auto& neighbor = neighbor_it->second;
			if (!isIndependent(neighbor_tag, world))
			{
				continue;
			}

			if (country.primary_religion == neighbor.primary_religion &&
				 (hasCrossRealmDynasticTie(ck3_world, country, neighbor) ||
					  (country.primary_culture == neighbor.primary_culture &&
						  std::abs(static_cast<int>(country.owned_core_locations.size()) -
								   static_cast<int>(neighbor.owned_core_locations.size())) <= 3)))
			{
				if (tag < neighbor_tag && alliance_counts[tag] < maxAllianceCount(country) &&
					 alliance_counts[neighbor_tag] < maxAllianceCount(neighbor) &&
					 relation_load[tag] + alliance_counts[tag] + guarantee_counts[tag] < maxDiplomaticRelations(country) &&
					 relation_load[neighbor_tag] + alliance_counts[neighbor_tag] + guarantee_counts[neighbor_tag] <
							 maxDiplomaticRelations(neighbor))
				{
					addScriptedRelation(world, tag, neighbor_tag, "alliance", true);
					++alliance_counts[tag];
					++alliance_counts[neighbor_tag];
				}
				addOpinion(world, tag, neighbor_tag, hasCrossRealmDynasticTie(ck3_world, country, neighbor) ? "opinion_dynastic_ties"
																																		 : "opinion_good_relations");
				addOpinion(world, neighbor_tag, tag, hasCrossRealmDynasticTie(ck3_world, country, neighbor) ? "opinion_dynastic_ties"
																																		 : "opinion_good_relations");
				continue;
			}

			const bool country_stronger =
					countryRankLevel(country.country_rank) > countryRankLevel(neighbor.country_rank) ||
					country.owned_core_locations.size() >= neighbor.owned_core_locations.size() * 2;
			if (country_stronger && country.primary_religion == neighbor.primary_religion &&
				 neighbor.country_rank == "rank_duchy")
			{
				if (guarantee_counts[tag] < maxGuaranteeCount(country) &&
					 relation_load[tag] + alliance_counts[tag] + guarantee_counts[tag] < maxDiplomaticRelations(country))
				{
					addScriptedRelation(world, tag, neighbor_tag, "guarantee", false);
					addOpinion(world, neighbor_tag, tag, "opinion_supportive_monarch");
					++guarantee_counts[tag];
				}
				continue;
			}

			if (country.primary_religion != neighbor.primary_religion ||
				 std::abs(country.belligerent_vs_conciliatory - neighbor.belligerent_vs_conciliatory) >= 25 ||
				 std::abs(static_cast<int>(country.owned_core_locations.size()) -
						  static_cast<int>(neighbor.owned_core_locations.size())) <= 2)
			{
				addRival(world, tag, neighbor_tag);
				addOpinion(world, tag, neighbor_tag, "border_aggression");
				if (country.primary_religion != neighbor.primary_religion)
				{
					addOpinion(world, tag, neighbor_tag, "opinion_threatens_conquest");
				}
			}
		}
	}
}

}  // namespace ck3eu5::convert
