#include "ck3/world.h"

#include "common/string_utils.h"

#include <algorithm>

namespace ck3eu5::ck3 {

TitleRank titleRankFromString(const std::string& value)
{
	const auto lowered = common::toLower(value);
	if (lowered == "barony")
	{
		return TitleRank::Barony;
	}
	if (lowered == "county")
	{
		return TitleRank::County;
	}
	if (lowered == "duchy")
	{
		return TitleRank::Duchy;
	}
	if (lowered == "kingdom")
	{
		return TitleRank::Kingdom;
	}
	if (lowered == "empire")
	{
		return TitleRank::Empire;
	}
	return TitleRank::Unknown;
}

TitleRank titleRankFromTitleKey(const std::string& title_key)
{
	if (title_key.rfind("b_", 0) == 0)
	{
		return TitleRank::Barony;
	}
	if (title_key.rfind("c_", 0) == 0)
	{
		return TitleRank::County;
	}
	if (title_key.rfind("d_", 0) == 0)
	{
		return TitleRank::Duchy;
	}
	if (title_key.rfind("k_", 0) == 0)
	{
		return TitleRank::Kingdom;
	}
	if (title_key.rfind("e_", 0) == 0)
	{
		return TitleRank::Empire;
	}
	return TitleRank::Unknown;
}

std::string toString(const TitleRank rank)
{
	switch (rank)
	{
		case TitleRank::Barony:
			return "barony";
		case TitleRank::County:
			return "county";
		case TitleRank::Duchy:
			return "duchy";
		case TitleRank::Kingdom:
			return "kingdom";
		case TitleRank::Empire:
			return "empire";
		case TitleRank::Unknown:
			break;
	}
	return "unknown";
}

const Dynasty* World::getDynasty(const std::string& id) const
{
	const auto it = dynasties.find(id);
	return it == dynasties.end() ? nullptr : &it->second;
}

const DynastyHouse* World::getDynastyHouse(const std::string& id) const
{
	const auto it = dynasty_houses.find(id);
	return it == dynasty_houses.end() ? nullptr : &it->second;
}

const Culture* World::getCulture(const std::string& id) const
{
	const auto it = cultures.find(id);
	return it == cultures.end() ? nullptr : &it->second;
}

const Faith* World::getFaith(const std::string& id) const
{
	const auto it = faiths.find(id);
	return it == faiths.end() ? nullptr : &it->second;
}

const Character* World::getCharacter(const std::string& id) const
{
	const auto it = characters.find(id);
	return it == characters.end() ? nullptr : &it->second;
}

Character* World::getCharacter(const std::string& id)
{
	const auto it = characters.find(id);
	return it == characters.end() ? nullptr : &it->second;
}

const Title* World::getTitle(const std::string& key) const
{
	const auto it = titles.find(key);
	return it == titles.end() ? nullptr : &it->second;
}

const Title* World::getTitleBySourceId(const std::string& source_id) const
{
	for (const auto& [key, title]: titles)
	{
		(void)key;
		if (title.source_id == source_id)
		{
			return &title;
		}
	}
	return nullptr;
}

const County* World::getCounty(const std::string& key) const
{
	const auto it = counties.find(key);
	return it == counties.end() ? nullptr : &it->second;
}

const County* World::findCountyByBaronyProvince(const std::string& province_id) const
{
	if (province_id.empty())
	{
		return nullptr;
	}
	for (const auto& [county_key, county]: counties)
	{
		(void)county_key;
		if (std::find(county.barony_province_keys.begin(), county.barony_province_keys.end(), province_id) != county.barony_province_keys.end())
		{
			return &county;
		}
	}
	return nullptr;
}

std::vector<std::string> World::heldTitleKeysOfCharacter(const std::string& id) const
{
	std::vector<std::string> result;
	const auto* character = getCharacter(id);
	if (!character)
	{
		return result;
	}

	auto append_unique = [&result](const std::string& title_key) {
		if (!title_key.empty() && std::find(result.begin(), result.end(), title_key) == result.end())
		{
			result.push_back(title_key);
		}
	};

	append_unique(character->primary_title);
	for (const auto& title_key: character->domain_titles)
	{
		append_unique(title_key);
	}
	for (const auto& title_key: character->held_titles)
	{
		append_unique(title_key);
	}
	return result;
}

std::vector<std::string> World::deJureCountyKeysOfTitle(const std::string& key) const
{
	if (const auto* title = getTitle(key))
	{
		return title->owned_de_jure_counties;
	}
	return {};
}

std::vector<std::string> World::deFactoCountyKeysOfTitle(const std::string& key) const
{
	if (const auto* title = getTitle(key))
	{
		return title->owned_de_facto_counties;
	}
	return {};
}

TitleRank World::primaryRankOfCharacter(const std::string& id) const
{
	const auto* character = getCharacter(id);
	if (!character)
	{
		return TitleRank::Unknown;
	}
	const auto* title = getTitle(primaryTitleOfCharacter(id));
	return title ? title->rank : TitleRank::Unknown;
}

std::string World::primaryTitleOfCharacter(const std::string& id) const
{
	const auto* character = getCharacter(id);
	if (!character)
	{
		return {};
	}
	if (!character->primary_title.empty())
	{
		return character->primary_title;
	}
	TitleRank best_rank = TitleRank::Unknown;
	std::string best_title;
	for (const auto& title_key: character->domain_titles)
	{
		const auto* title = getTitle(title_key);
		if (!title)
		{
			continue;
		}
		if (static_cast<int>(title->rank) > static_cast<int>(best_rank))
		{
			best_rank = title->rank;
			best_title = title->key;
		}
	}
	for (const auto& title_key: character->held_titles)
	{
		const auto* title = getTitle(title_key);
		if (!title)
		{
			continue;
		}
		if (static_cast<int>(title->rank) > static_cast<int>(best_rank))
		{
			best_rank = title->rank;
			best_title = title->key;
		}
	}
	return best_title;
}

std::string World::topLiegeOfCharacter(const std::string& id) const
{
	std::string current = id;
	std::vector<std::string> seen;
	while (!current.empty())
	{
		if (std::find(seen.begin(), seen.end(), current) != seen.end())
		{
			break;
		}
		seen.push_back(current);
		const auto* character = getCharacter(current);
		if (!character || character->liege.empty())
		{
			break;
		}
		current = character->liege;
	}
	return current;
}

}  // namespace ck3eu5::ck3
