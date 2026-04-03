#include "ck3/installed_world_enricher.h"

#include "common/string_utils.h"

#include <algorithm>
#include <cctype>

namespace ck3eu5::ck3 {

namespace {

std::string deriveDisplayName(const std::string& key)
{
	std::string stripped = key;
	for (const std::string_view prefix: {"b_", "c_", "d_", "k_", "e_"})
	{
		if (stripped.rfind(prefix, 0) == 0)
		{
			stripped = stripped.substr(prefix.size());
			break;
		}
	}
	for (char& c: stripped)
	{
		if (c == '_')
		{
			c = ' ';
		}
	}
	if (!stripped.empty())
	{
		stripped[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(stripped[0])));
	}
	return stripped;
}

void appendUnique(std::vector<std::string>& values, const std::string& value)
{
	if (!value.empty() && std::find(values.begin(), values.end(), value) == values.end())
	{
		values.push_back(value);
	}
}

bool shouldReplaceDisplayName(const std::string& current, const std::string& key)
{
	return current.empty() || current == key || common::toLower(current) == common::toLower(deriveDisplayName(key));
}

void enrichDynasties(World& world, const InstalledTitles& installed_data)
{
	for (auto& [id, dynasty]: world.dynasties)
	{
		(void)id;
		const auto localized_key = installed_data.resolveLocalization(dynasty.key);
		if (shouldReplaceDisplayName(dynasty.display_name, dynasty.key) && localized_key != dynasty.key)
		{
			dynasty.display_name = localized_key;
		}
	}

	for (auto& [id, house]: world.dynasty_houses)
	{
		(void)id;
		const auto localized_key = installed_data.resolveLocalization(house.key);
		const auto localized_name = installed_data.resolveLocalization(house.localized_name);
		const auto direct_name = installed_data.resolveLocalization(house.name);
		const auto localized_prefix = installed_data.resolveLocalization(house.prefix);
		const auto resolved_localized_name = !localized_name.empty() && localized_name != house.localized_name;
		const auto resolved_direct_name = !direct_name.empty() && direct_name != house.name;
		const auto resolved_localized_key = !localized_key.empty() && localized_key != house.key;

		if (resolved_localized_name)
		{
			house.localized_name = localized_name;
		}
		if (!localized_prefix.empty() && localized_prefix != house.prefix)
		{
			house.prefix = localized_prefix;
		}
		if (resolved_direct_name)
		{
			house.name = direct_name;
		}
		if (shouldReplaceDisplayName(house.name, house.key))
		{
			if (resolved_localized_name)
			{
				house.name = localized_name;
			}
			else if (resolved_localized_key)
			{
				house.name = localized_key;
			}
		}
		if (house.name.empty() && !house.localized_name.empty())
		{
			house.name = house.localized_name;
		}
		if (house.name.empty())
		{
			if (const auto* dynasty = world.getDynasty(house.dynasty_id))
			{
				house.name = dynasty->display_name;
			}
		}
	}
}

void enrichCultures(World& world, const InstalledTitles& installed_data)
{
	for (auto& [id, culture]: world.cultures)
	{
		(void)id;
		if (const auto installed = installed_data.cultures.find(culture.key); installed != installed_data.cultures.end())
		{
			if (shouldReplaceDisplayName(culture.display_name, culture.key))
			{
				culture.display_name = installed->second.display_name;
			}
			if (culture.ethos.empty())
			{
				culture.ethos = installed->second.ethos;
			}
			if (culture.heritage.empty())
			{
				culture.heritage = installed->second.heritage;
			}
			if (culture.language.empty())
			{
				culture.language = installed->second.language;
			}
			if (culture.parents.empty())
			{
				culture.parents = installed->second.parents;
			}
		}
		else
		{
			const auto localized = installed_data.resolveLocalization(culture.key);
			if (shouldReplaceDisplayName(culture.display_name, culture.key) && localized != culture.key)
			{
				culture.display_name = localized;
			}
		}
	}
}

void enrichFaiths(World& world, const InstalledTitles& installed_data)
{
	for (auto& [id, faith]: world.faiths)
	{
		(void)id;
		if (const auto installed = installed_data.faiths.find(faith.key); installed != installed_data.faiths.end())
		{
			if (shouldReplaceDisplayName(faith.display_name, faith.key))
			{
				faith.display_name = installed->second.display_name;
			}
			if (faith.religion.empty())
			{
				faith.religion = installed->second.religion_key;
			}
			if (faith.religion_display_name.empty())
			{
				faith.religion_display_name = installed->second.religion_display_name;
			}
			if (faith.religion_family.empty())
			{
				faith.religion_family = installed->second.religion_family;
			}
			if (faith.doctrines.empty())
			{
				faith.doctrines = installed->second.doctrines;
			}
		}
		else
		{
			const auto localized = installed_data.resolveLocalization(faith.key);
			if (shouldReplaceDisplayName(faith.display_name, faith.key) && localized != faith.key)
			{
				faith.display_name = localized;
			}
		}
	}
}

void enrichTitles(World& world, const InstalledTitles& installed_data)
{
	for (auto& [key, title]: world.titles)
	{
		if (const auto installed = installed_data.titles.find(key); installed != installed_data.titles.end())
		{
			if (shouldReplaceDisplayName(title.display_name, title.key))
			{
				title.display_name = installed->second.display_name;
			}
			if (title.rank == TitleRank::Unknown)
			{
				title.rank = installed->second.rank;
			}
			if (title.de_jure_liege_title.empty())
			{
				title.de_jure_liege_title = installed->second.de_jure_liege_title;
			}
			if (title.de_jure_vassals.empty())
			{
				title.de_jure_vassals = installed->second.de_jure_vassals;
			}
			else
			{
				for (const auto& vassal: installed->second.de_jure_vassals)
				{
					appendUnique(title.de_jure_vassals, vassal);
				}
			}
			if (title.capital_province.empty() && installed->second.province_id > 0)
			{
				title.capital_province = std::to_string(installed->second.province_id);
			}
		}
		else
		{
			const auto localized = installed_data.resolveLocalization(key);
			if (shouldReplaceDisplayName(title.display_name, title.key) && localized != key)
			{
				title.display_name = localized;
			}
		}
	}
}

void enrichCounties(World& world, const InstalledTitles& installed_data)
{
	for (auto& [key, county]: world.counties)
	{
		const auto installed = installed_data.counties.find(key);
		if (installed == installed_data.counties.end())
		{
			const auto localized = installed_data.resolveLocalization(key);
			if (shouldReplaceDisplayName(county.display_name, county.key) && localized != key)
			{
				county.display_name = localized;
			}
			continue;
		}

		if (shouldReplaceDisplayName(county.display_name, county.key))
		{
			county.display_name = installed->second.display_name;
		}

		if (county.barony_keys.empty())
		{
			for (const auto& barony: installed->second.baronies)
			{
				county.barony_keys.push_back(barony.key);
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

		for (size_t index = 0; index < installed->second.baronies.size(); ++index)
		{
			const auto& barony = installed->second.baronies[index];
			const auto existing = std::find(county.barony_keys.begin(), county.barony_keys.end(), barony.key);
			if (existing == county.barony_keys.end())
			{
				county.barony_keys.push_back(barony.key);
				county.barony_display_names.push_back(barony.display_name);
				county.barony_province_keys.push_back(barony.province_id > 0 ? std::to_string(barony.province_id) : std::string{});
				continue;
			}

			const auto offset = static_cast<size_t>(std::distance(county.barony_keys.begin(), existing));
			if (offset >= county.barony_display_names.size())
			{
				county.barony_display_names.resize(offset + 1);
			}
			if (offset >= county.barony_province_keys.size())
			{
				county.barony_province_keys.resize(offset + 1);
			}
			if (county.barony_display_names[offset].empty())
			{
				county.barony_display_names[offset] = barony.display_name;
			}
			if (county.barony_province_keys[offset].empty() && barony.province_id > 0)
			{
				county.barony_province_keys[offset] = std::to_string(barony.province_id);
			}
		}
	}
}

}  // namespace

void InstalledWorldEnricher::enrich(World& world, const InstalledTitles& installed_data) const
{
	enrichDynasties(world, installed_data);
	enrichCultures(world, installed_data);
	enrichFaiths(world, installed_data);
	enrichTitles(world, installed_data);
	enrichCounties(world, installed_data);
}

}  // namespace ck3eu5::ck3
