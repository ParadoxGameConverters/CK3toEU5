#include "ck3/installed_titles.h"
#include "ck3/installed_world_enricher.h"
#include "ck3/raw_save_normalizer.h"
#include "ck3/world.h"
#include "ck3/world_importer.h"
#include "diagnostics/diagnostics_report.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct RealSaveExpectation
{
	std::string filename;
	std::optional<std::string> expected_ruler_name;
};

struct GraphStats
{
	size_t dead_characters = 0;
	size_t characters_with_claims = 0;
	size_t characters_with_domain_titles = 0;
	size_t characters_with_primary_titles = 0;
	size_t characters_with_lieges = 0;
	size_t reciprocal_spouse_pairs = 0;
	size_t titles_with_notables = 0;
	size_t titles_with_previous_holders = 0;
	size_t resolved_dead_previous_holders = 0;
	size_t counties_with_baronies = 0;
	size_t titles_with_capital_counties = 0;
	size_t titles_with_capital_provinces = 0;
	size_t titles_with_owned_de_jure_counties = 0;
};

bool contains(const std::vector<std::string>& values, const std::string& needle)
{
	return std::find(values.begin(), values.end(), needle) != values.end();
}

std::optional<fs::path> locateSaveDirectory()
{
	const auto* user_profile = std::getenv("USERPROFILE");
	if (!user_profile)
	{
		return std::nullopt;
	}

	const std::vector<fs::path> candidates = {
		 fs::path(user_profile) / "OneDrive/Documents/Paradox Interactive/Crusader Kings III/save games",
		 fs::path(user_profile) / "Documents/Paradox Interactive/Crusader Kings III/save games"};

	for (const auto& candidate: candidates)
	{
		if (fs::exists(candidate))
		{
			return candidate;
		}
	}
	return std::nullopt;
}

std::optional<fs::path> locateGameDirectory()
{
	if (const auto* program_files_x86 = std::getenv("ProgramFiles(x86)"))
	{
		const auto candidate = fs::path(program_files_x86) / "Steam/steamapps/common/Crusader Kings III";
		if (fs::exists(candidate))
		{
			return candidate;
		}
	}

	const fs::path fallback = "C:/Program Files (x86)/Steam/steamapps/common/Crusader Kings III";
	if (fs::exists(fallback))
	{
		return fallback;
	}

	return std::nullopt;
}

ck3eu5::ck3::World importRealSave(const fs::path& save_path, ck3eu5::diagnostics::DiagnosticsReport& diagnostics)
{
	ck3eu5::ck3::WorldImporter importer;
	const auto normalized = ck3eu5::ck3::normalizeSaveFile(save_path);
	return importer.importText(normalized, diagnostics);
}

const ck3eu5::ck3::Character* findNamedRuler(const ck3eu5::ck3::World& world, const std::string& first_name)
{
	for (const auto& [id, character]: world.characters)
	{
		(void)id;
		if (character.first_name != first_name || character.dead)
		{
			continue;
		}
		if (!world.primaryTitleOfCharacter(character.id).empty() || !world.heldTitleKeysOfCharacter(character.id).empty())
		{
			return &character;
		}
	}

	for (const auto& [id, character]: world.characters)
	{
		(void)id;
		if (character.first_name == first_name)
		{
			return &character;
		}
	}

	return nullptr;
}

GraphStats collectGraphStats(const ck3eu5::ck3::World& world)
{
	GraphStats stats;

	for (const auto& [id, character]: world.characters)
	{
		(void)id;
		if (character.dead)
		{
			++stats.dead_characters;
		}
		if (!character.claims.empty())
		{
			++stats.characters_with_claims;
		}
		if (!character.domain_titles.empty())
		{
			++stats.characters_with_domain_titles;
			for (const auto& title_key: character.domain_titles)
			{
				assert(world.getTitle(title_key));
			}
		}
		if (!character.primary_title.empty())
		{
			++stats.characters_with_primary_titles;
			const auto* primary_title = world.getTitle(character.primary_title);
			assert(primary_title);
			assert(primary_title->holder_id.empty() || primary_title->holder_id == character.id);
		}
		if (!character.liege.empty())
		{
			++stats.characters_with_lieges;
			assert(world.getCharacter(character.liege));
			assert(character.liege != character.id);
			if (!character.primary_title.empty())
			{
				if (const auto* primary_title = world.getTitle(character.primary_title);
					 primary_title && !primary_title->de_facto_liege_title.empty())
				{
					if (const auto* liege_title = world.getTitle(primary_title->de_facto_liege_title);
						 liege_title && !liege_title->holder_id.empty() && liege_title->holder_id != character.id)
					{
						assert(character.liege == liege_title->holder_id);
					}
				}
			}
		}
		if (!character.spouse_id.empty())
		{
			const auto* spouse = world.getCharacter(character.spouse_id);
			if (spouse && spouse->spouse_id == character.id && character.id < spouse->id)
			{
				++stats.reciprocal_spouse_pairs;
			}
		}
		for (const auto& claim: character.claims)
		{
			assert(world.getTitle(claim));
		}
	}

	for (const auto& [key, title]: world.titles)
	{
		(void)key;
		if (!title.holder_id.empty())
		{
			const auto* holder = world.getCharacter(title.holder_id);
			assert(holder);
			assert(contains(holder->held_titles, title.key));
		}
		if (!title.capital_county.empty())
		{
			++stats.titles_with_capital_counties;
			assert(world.getCounty(title.capital_county));
		}
		if (!title.capital_province.empty())
		{
			++stats.titles_with_capital_provinces;
		}
		if (!title.owned_de_jure_counties.empty())
		{
			++stats.titles_with_owned_de_jure_counties;
		}
		if (!title.heirs.empty() || !title.claimants.empty() || !title.electors.empty())
		{
			++stats.titles_with_notables;
		}
		if (!title.previous_holders.empty())
		{
			++stats.titles_with_previous_holders;
			for (const auto& previous_holder_id: title.previous_holders)
			{
				if (const auto* previous_holder = world.getCharacter(previous_holder_id);
					 previous_holder && previous_holder->dead && !previous_holder->death_date.empty())
				{
					++stats.resolved_dead_previous_holders;
				}
			}
		}
	}

	for (const auto& [key, county]: world.counties)
	{
		(void)key;
		if (!county.barony_keys.empty())
		{
			++stats.counties_with_baronies;
		}
		if (!county.owner_id.empty())
		{
			assert(world.getCharacter(county.owner_id));
			assert(world.topLiegeOfCharacter(county.owner_id) == county.top_liege_id);
		}
	}

	return stats;
}

void assertStableVanillaGraph(const ck3eu5::ck3::World& world)
{
	const auto* england = world.getTitle("k_england");
	const auto* middlesex_title = world.getTitle("c_middlesex");
	const auto* london = world.getTitle("b_london");
	const auto* middlesex_county = world.getCounty("c_middlesex");

	assert(england);
	assert(middlesex_title);
	assert(london);
	assert(middlesex_county);
	assert(england->rank == ck3eu5::ck3::TitleRank::Kingdom);
	assert(middlesex_title->rank == ck3eu5::ck3::TitleRank::County);
	assert(london->rank == ck3eu5::ck3::TitleRank::Barony);
	assert(contains(middlesex_title->de_jure_vassals, "b_london"));
	assert(contains(middlesex_county->barony_keys, "b_london"));
	assert(contains(england->owned_de_jure_counties, "c_middlesex"));
}

void runRealSaveRegression(const fs::path& save_path,
	 const std::optional<std::string>& expected_ruler_name,
	 const ck3eu5::ck3::InstalledTitles* installed_data)
{
	ck3eu5::diagnostics::DiagnosticsReport diagnostics;
	auto world = importRealSave(save_path, diagnostics);
	std::cout << "  sizes: characters=" << world.characters.size() << " titles=" << world.titles.size()
			  << " counties=" << world.counties.size() << " houses=" << world.dynasty_houses.size()
			  << " cultures=" << world.cultures.size() << " faiths=" << world.faiths.size() << std::endl;

	assert(!diagnostics.hasErrors());
	assert(world.date != "1337.1.1");
	assert(world.characters.size() > 1000);
	assert(world.titles.size() > 10000);
	assert(world.counties.size() > 1000);
	assert(world.dynasty_houses.size() > 1000);
	assert(world.cultures.size() > 50);
	assert(world.faiths.size() > 20);

	if (installed_data)
	{
		ck3eu5::ck3::InstalledWorldEnricher enricher;
		enricher.enrich(world, *installed_data);

		assert(world.titles.at("c_middlesex").display_name == "Middlesex");
		assert(world.titles.at("b_london").display_name == "London");
		assert(!world.titles.at("b_london").capital_province.empty());

		size_t enriched_cultures = 0;
		for (const auto& [id, culture]: world.cultures)
		{
			(void)id;
			if (!culture.heritage.empty() || !culture.language.empty() || !culture.ethos.empty())
			{
				++enriched_cultures;
			}
		}
		size_t enriched_faiths = 0;
		for (const auto& [id, faith]: world.faiths)
		{
			(void)id;
			if (!faith.religion_family.empty() || !faith.religion_display_name.empty() || !faith.doctrines.empty())
			{
				++enriched_faiths;
			}
		}
		assert(enriched_cultures > 10);
		assert(enriched_faiths > 10);
	}

	assertStableVanillaGraph(world);

	const auto stats = collectGraphStats(world);
	std::cout << "  stats: dead=" << stats.dead_characters << " claims=" << stats.characters_with_claims
			  << " domain_holders=" << stats.characters_with_domain_titles
			  << " primaries=" << stats.characters_with_primary_titles << " lieged=" << stats.characters_with_lieges
			  << " spouse_pairs=" << stats.reciprocal_spouse_pairs << " notables=" << stats.titles_with_notables
			  << " previous_holders=" << stats.titles_with_previous_holders
			  << " resolved_dead_previous_holders=" << stats.resolved_dead_previous_holders
			  << " counties_with_baronies=" << stats.counties_with_baronies << " capital_counties="
			  << stats.titles_with_capital_counties << " capital_provinces=" << stats.titles_with_capital_provinces
			  << " titles_with_owned_de_jure_counties=" << stats.titles_with_owned_de_jure_counties << std::endl;
	assert(stats.dead_characters > 0);
	assert(stats.characters_with_claims > 0);
	assert(stats.characters_with_domain_titles > 100);
	assert(stats.characters_with_primary_titles > 100);
	assert(stats.characters_with_lieges > 100);
	assert(stats.reciprocal_spouse_pairs > 10);
	assert(stats.titles_with_notables > 100);
	assert(stats.counties_with_baronies > world.counties.size() / 2);
	assert(stats.titles_with_capital_counties > world.counties.size() / 2);
	assert(stats.titles_with_capital_provinces > world.counties.size() / 2);
	assert(stats.titles_with_owned_de_jure_counties > 1000);

	if (expected_ruler_name)
	{
		const auto* ruler = findNamedRuler(world, *expected_ruler_name);
		assert(ruler);
		assert(!world.primaryTitleOfCharacter(ruler->id).empty() || !world.heldTitleKeysOfCharacter(ruler->id).empty());
	}
}

}  // namespace

int main()
{
	const auto save_directory = locateSaveDirectory();
	if (!save_directory)
	{
		std::cout << "Real save regression skipped: CK3 save directory not found.\n";
		return 0;
	}

	std::optional<ck3eu5::ck3::InstalledTitles> installed_data;
	if (const auto game_directory = locateGameDirectory())
	{
		installed_data = ck3eu5::ck3::InstalledTitlesLoader().load(*game_directory);
	}

	const std::vector<RealSaveExpectation> saves = {
		 {"Vanilla_probe.ck3", std::nullopt},
		 {"King_Antigonos_867_01_01.ck3", std::string("Antigonos")},
		 {"Shah_Megabyzus_867_05_05.ck3", std::string("Megabyzus")}};

	size_t executed = 0;
	for (const auto& save: saves)
	{
		const auto save_path = *save_directory / save.filename;
		if (!fs::exists(save_path))
		{
			std::cout << "Skipping missing real save fixture: " << save.filename << '\n';
			continue;
		}

		std::cout << "Running real save regression: " << save.filename << '\n';
		runRealSaveRegression(save_path, save.expected_ruler_name, installed_data ? &*installed_data : nullptr);
		++executed;
	}

	assert(executed >= 2);
	std::cout << "Real save regression passed.\n";
	return 0;
}
