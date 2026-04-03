#include "ck3/realm_normalizer.h"

#include <algorithm>
#include <set>

namespace ck3eu5::ck3 {

namespace {

void appendUnique(std::vector<std::string>& values, const std::string& value)
{
	if (!value.empty() && std::find(values.begin(), values.end(), value) == values.end())
	{
		values.push_back(value);
	}
}

template <class Predicate>
void dedupeAndFilter(std::vector<std::string>& values, Predicate&& predicate)
{
	std::vector<std::string> filtered;
	filtered.reserve(values.size());
	for (const auto& value: values)
	{
		if (value.empty() || !predicate(value))
		{
			continue;
		}
		appendUnique(filtered, value);
	}
	values = std::move(filtered);
}

std::string firstNonEmptyProvince(const County& county)
{
	if (!county.province_key.empty())
	{
		return county.province_key;
	}
	for (const auto& province: county.barony_province_keys)
	{
		if (!province.empty())
		{
			return province;
		}
	}
	return {};
}

const County* findCountyContainingBarony(const World& world, const std::string& barony_key)
{
	for (const auto& [county_key, county]: world.counties)
	{
		(void)county_key;
		if (std::find(county.barony_keys.begin(), county.barony_keys.end(), barony_key) != county.barony_keys.end())
		{
			return &county;
		}
	}
	return nullptr;
}

std::vector<std::string> collectCountyKeys(const World& world,
	 const std::string& title_key,
	 const bool de_jure,
	 std::set<std::string>& seen_titles)
{
	if (title_key.empty() || !seen_titles.insert(title_key).second)
	{
		return {};
	}

	const auto* title = world.getTitle(title_key);
	if (!title)
	{
		return {};
	}

	if (title->rank == TitleRank::County && world.getCounty(title_key))
	{
		return {title_key};
	}

	std::vector<std::string> result;
	const auto& vassals = de_jure ? title->de_jure_vassals : title->de_facto_vassals;
	for (const auto& vassal_key: vassals)
	{
		for (const auto& county_key: collectCountyKeys(world, vassal_key, de_jure, seen_titles))
		{
			appendUnique(result, county_key);
		}
	}
	return result;
}

int titleScore(const Title& title, const Character& character)
{
	int score = static_cast<int>(title.rank) * 100000;
	score += static_cast<int>(title.owned_de_facto_counties.size()) * 200;
	score += static_cast<int>(title.owned_de_jure_counties.size()) * 50;
	if (!title.capital_county.empty())
	{
		score += 25;
	}
	if (!title.capital_province.empty())
	{
		score += 10;
	}
	if (!character.realm_capital_province.empty() && character.realm_capital_province == title.capital_province)
	{
		score += 1000;
	}
	return score;
}

std::string choosePrimaryTitle(const World& world, const Character& character)
{
	const auto held_titles = world.heldTitleKeysOfCharacter(character.id);
	const Title* best_title = nullptr;
	int best_score = -1;
	for (const auto& title_key: held_titles)
	{
		const auto* title = world.getTitle(title_key);
		if (!title || title->rank == TitleRank::Unknown)
		{
			continue;
		}
		const auto score = titleScore(*title, character);
		if (!best_title || score > best_score)
		{
			best_title = title;
			best_score = score;
		}
	}
	return best_title ? best_title->key : std::string{};
}

void normalizeTitles(World& world)
{
	for (auto& [title_key, title]: world.titles)
	{
		(void)title_key;
		if (!title.holder_id.empty() && !world.getCharacter(title.holder_id))
		{
			title.holder_id.clear();
		}
		if (title.de_facto_liege_title == title.key || !world.getTitle(title.de_facto_liege_title))
		{
			title.de_facto_liege_title.clear();
		}
		if (title.de_jure_liege_title == title.key)
		{
			title.de_jure_liege_title.clear();
		}

		dedupeAndFilter(title.de_jure_vassals, [&title](const std::string& key) { return key != title.key; });
		dedupeAndFilter(title.de_facto_vassals, [&title](const std::string& key) { return key != title.key; });
		dedupeAndFilter(title.heirs, [&world, &title](const std::string& id) {
			const auto* character = world.getCharacter(id);
			return character && !character->dead && id != title.holder_id;
		});
		dedupeAndFilter(title.claimants, [&world, &title](const std::string& id) {
			const auto* character = world.getCharacter(id);
			return character && !character->dead && id != title.holder_id;
		});
		dedupeAndFilter(title.electors, [&world](const std::string& id) { return world.getCharacter(id) != nullptr; });
		dedupeAndFilter(title.previous_holders, [&title](const std::string& id) { return id != title.holder_id; });
	}

	std::map<std::string, std::vector<std::string>> de_facto_vassals;
	for (const auto& [title_key, title]: world.titles)
	{
		if (!title.de_facto_liege_title.empty())
		{
			appendUnique(de_facto_vassals[title.de_facto_liege_title], title_key);
		}
		if (!title.de_jure_liege_title.empty())
		{
			if (auto* liege = const_cast<Title*>(world.getTitle(title.de_jure_liege_title)); liege)
			{
				appendUnique(liege->de_jure_vassals, title_key);
			}
		}
	}

	for (auto& [title_key, title]: world.titles)
	{
		if (const auto it = de_facto_vassals.find(title_key); it != de_facto_vassals.end())
		{
			title.de_facto_vassals = it->second;
		}
		else
		{
			title.de_facto_vassals.clear();
		}
	}
}

void normalizeCharacters(World& world)
{
	for (auto& [character_id, character]: world.characters)
	{
		(void)character_id;
		dedupeAndFilter(character.domain_titles, [&world](const std::string& title_key) {
			const auto* title = world.getTitle(title_key);
			return title && title->rank <= TitleRank::County;
		});
		dedupeAndFilter(character.held_titles, [&world](const std::string& title_key) { return world.getTitle(title_key) != nullptr; });
		dedupeAndFilter(character.claims, [&world](const std::string& title_key) { return world.getTitle(title_key) != nullptr; });
		if (character.liege == character.id)
		{
			character.liege.clear();
		}
		if (character.employer_id == character.id)
		{
			character.employer_id.clear();
		}
		if (character.suzerain_id == character.id)
		{
			character.suzerain_id.clear();
		}
	}

	for (auto& [character_id, character]: world.characters)
	{
		(void)character_id;
		character.held_titles.clear();
	}

	for (const auto& [title_key, title]: world.titles)
	{
		if (title.holder_id.empty())
		{
			continue;
		}
		auto* holder = world.getCharacter(title.holder_id);
		if (!holder)
		{
			continue;
		}
		appendUnique(holder->held_titles, title_key);
		if (title.rank <= TitleRank::County)
		{
			appendUnique(holder->domain_titles, title_key);
		}
	}
}

void recomputeTitleCountyCaches(World& world)
{
	for (auto& [title_key, title]: world.titles)
	{
		std::set<std::string> seen_de_jure_titles;
		std::set<std::string> seen_de_facto_titles;
		title.owned_de_jure_counties = collectCountyKeys(world, title_key, true, seen_de_jure_titles);
		title.owned_de_facto_counties = collectCountyKeys(world, title_key, false, seen_de_facto_titles);
	}
}

void normalizeTitleCapitals(World& world)
{
	for (auto& [county_key, county]: world.counties)
	{
		(void)county_key;
		if (county.barony_keys.empty())
		{
			if (const auto* county_title = world.getTitle(county.key))
			{
				for (const auto& vassal_key: county_title->de_jure_vassals)
				{
					const auto* barony = world.getTitle(vassal_key);
					if (!barony || barony->rank != TitleRank::Barony)
					{
						continue;
					}
					county.barony_keys.push_back(barony->key);
					county.barony_display_names.push_back(barony->display_name);
					county.barony_province_keys.push_back(barony->capital_province);
				}
			}
		}
		if (county.barony_display_names.size() < county.barony_keys.size())
		{
			county.barony_display_names.resize(county.barony_keys.size());
		}
		if (county.barony_province_keys.size() < county.barony_keys.size())
		{
			county.barony_province_keys.resize(county.barony_keys.size());
		}

		for (size_t index = 0; index < county.barony_keys.size(); ++index)
		{
			const auto* barony = world.getTitle(county.barony_keys[index]);
			if (!barony)
			{
				continue;
			}
			if (county.barony_display_names[index].empty())
			{
				county.barony_display_names[index] = barony->display_name;
			}
			if (county.barony_province_keys[index].empty() && !barony->capital_province.empty())
			{
				county.barony_province_keys[index] = barony->capital_province;
			}
		}

		if (county.province_key.empty())
		{
			county.province_key = firstNonEmptyProvince(county);
		}
	}

	for (auto& [title_key, title]: world.titles)
	{
		if (!title.capital_county.empty() && !world.getCounty(title.capital_county))
		{
			title.capital_county.clear();
		}
		if (title.rank == TitleRank::Barony)
		{
			if (title.capital_county.empty())
			{
				if (const auto* county = findCountyContainingBarony(world, title_key))
				{
					title.capital_county = county->key;
				}
			}
			if (title.capital_province.empty())
			{
				if (const auto* county = findCountyContainingBarony(world, title_key))
				{
					const auto it = std::find(county->barony_keys.begin(), county->barony_keys.end(), title_key);
					if (it != county->barony_keys.end())
					{
						const auto index = static_cast<size_t>(std::distance(county->barony_keys.begin(), it));
						if (index < county->barony_province_keys.size())
						{
							title.capital_province = county->barony_province_keys[index];
						}
					}
				}
			}
			continue;
		}

		if (title.rank == TitleRank::County)
		{
			if (world.getCounty(title_key))
			{
				title.capital_county = title_key;
				if (title.capital_province.empty())
				{
					title.capital_province = firstNonEmptyProvince(*world.getCounty(title_key));
				}
			}
			continue;
		}

		std::string chosen_capital_county = title.capital_county;
		if (chosen_capital_county.empty() || !world.getCounty(chosen_capital_county))
		{
			if (!title.capital_province.empty())
			{
				if (const auto* county = world.findCountyByBaronyProvince(title.capital_province))
				{
					chosen_capital_county = county->key;
				}
			}
		}
		if ((chosen_capital_county.empty() || !world.getCounty(chosen_capital_county)) && !title.holder_id.empty())
		{
			if (const auto* holder = world.getCharacter(title.holder_id); holder && !holder->realm_capital_province.empty())
			{
				if (const auto* county = world.findCountyByBaronyProvince(holder->realm_capital_province))
				{
					if (std::find(title.owned_de_facto_counties.begin(), title.owned_de_facto_counties.end(), county->key) != title.owned_de_facto_counties.end() ||
						 std::find(title.owned_de_jure_counties.begin(), title.owned_de_jure_counties.end(), county->key) != title.owned_de_jure_counties.end())
					{
						chosen_capital_county = county->key;
					}
				}
			}
		}
		if ((chosen_capital_county.empty() || !world.getCounty(chosen_capital_county)) && !title.owned_de_facto_counties.empty())
		{
			chosen_capital_county = title.owned_de_facto_counties.front();
		}
		if ((chosen_capital_county.empty() || !world.getCounty(chosen_capital_county)) && !title.owned_de_jure_counties.empty())
		{
			chosen_capital_county = title.owned_de_jure_counties.front();
		}
		if ((chosen_capital_county.empty() || !world.getCounty(chosen_capital_county)))
		{
			for (const auto& vassal_key: title.de_jure_vassals)
			{
				if (const auto* vassal = world.getTitle(vassal_key); vassal && vassal->rank == TitleRank::County && world.getCounty(vassal_key))
				{
					chosen_capital_county = vassal_key;
					break;
				}
			}
		}

		if (!chosen_capital_county.empty() && world.getCounty(chosen_capital_county))
		{
			title.capital_county = chosen_capital_county;
			if (title.capital_province.empty())
			{
				title.capital_province = firstNonEmptyProvince(*world.getCounty(chosen_capital_county));
			}
		}
	}
}

void normalizePrimaryTitlesAndLieges(World& world)
{
	for (auto& [character_id, character]: world.characters)
	{
		(void)character_id;
		const auto chosen_primary_title = choosePrimaryTitle(world, character);
		if (!chosen_primary_title.empty())
		{
			character.primary_title = chosen_primary_title;
		}
		else if (!character.primary_title.empty() && !world.getTitle(character.primary_title))
		{
			character.primary_title.clear();
		}

		if (character.realm_capital_province.empty() && !character.primary_title.empty())
		{
			if (const auto* primary_title = world.getTitle(character.primary_title))
			{
				character.realm_capital_province = primary_title->capital_province;
				if (character.government.empty())
				{
					character.government = primary_title->government;
				}
			}
		}
		if (character.realm_capital_province.empty())
		{
			for (const auto& title_key: character.domain_titles)
			{
				const auto* title = world.getTitle(title_key);
				if (title && !title->capital_province.empty())
				{
					character.realm_capital_province = title->capital_province;
					break;
				}
			}
		}

		std::string derived_liege;
		if (!character.primary_title.empty())
		{
			if (const auto* primary_title = world.getTitle(character.primary_title);
				 primary_title && !primary_title->de_facto_liege_title.empty())
			{
				if (const auto* liege_title = world.getTitle(primary_title->de_facto_liege_title);
					 liege_title && !liege_title->holder_id.empty() && liege_title->holder_id != character.id)
				{
					derived_liege = liege_title->holder_id;
				}
			}
		}
		if (derived_liege.empty() && !character.suzerain_id.empty() && character.suzerain_id != character.id &&
			 world.getCharacter(character.suzerain_id))
		{
			derived_liege = character.suzerain_id;
		}
		if (derived_liege.empty() && !character.liege.empty() && character.liege != character.id && world.getCharacter(character.liege))
		{
			derived_liege = character.liege;
		}
		if (derived_liege.empty() && !character.employer_id.empty() && character.employer_id != character.id &&
			 world.getCharacter(character.employer_id) && world.heldTitleKeysOfCharacter(character.id).empty())
		{
			derived_liege = character.employer_id;
		}
		character.liege = derived_liege;
	}
}

void normalizeCounties(World& world)
{
	for (auto& [county_key, county]: world.counties)
	{
		const auto* title = world.getTitle(county_key);
		if (county.source_title_id.empty() && title && !title->source_id.empty())
		{
			county.source_title_id = title->source_id;
		}
		if (county.owner_id.empty() && title)
		{
			county.owner_id = title->holder_id;
		}
		if (county.government.empty() && title)
		{
			county.government = title->government;
		}
		if (county.government.empty() && !county.owner_id.empty())
		{
			if (const auto* owner = world.getCharacter(county.owner_id))
			{
				county.government = owner->government;
			}
		}
		if (county.display_name.empty() && title)
		{
			county.display_name = title->display_name;
		}
		if (county.province_key.empty())
		{
			if (title && !title->capital_province.empty())
			{
				county.province_key = title->capital_province;
			}
			if (county.province_key.empty())
			{
				county.province_key = firstNonEmptyProvince(county);
			}
		}
		if (county.top_liege_id.empty() || !world.getCharacter(county.top_liege_id))
		{
			if (!county.owner_id.empty())
			{
				county.top_liege_id = world.topLiegeOfCharacter(county.owner_id);
			}
		}
	}
}

void propagateClaimsFromTitles(World& world)
{
	for (auto& [title_key, title]: world.titles)
	{
		(void)title_key;
		for (const auto& claimant_id: title.claimants)
		{
			if (auto* claimant = world.getCharacter(claimant_id))
			{
				appendUnique(claimant->claims, title.key);
			}
		}
	}

	for (auto& [character_id, character]: world.characters)
	{
		(void)character_id;
		dedupeAndFilter(character.claims, [&world](const std::string& title_key) { return world.getTitle(title_key) != nullptr; });
	}
}

}  // namespace

void RealmNormalizer::normalize(World& world) const
{
	normalizeTitles(world);
	normalizeCharacters(world);
	recomputeTitleCountyCaches(world);
	normalizeTitleCapitals(world);
	normalizePrimaryTitlesAndLieges(world);
	normalizeCounties(world);
	propagateClaimsFromTitles(world);
}

}  // namespace ck3eu5::ck3
