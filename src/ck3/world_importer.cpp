#include "ck3/world_importer.h"

#include "ck3/installed_titles.h"
#include "ck3/installed_world_enricher.h"
#include "ck3/input_reader.h"
#include "ck3/realm_normalizer.h"
#include "common/logger.h"
#include "common/pds_parser.h"
#include "common/string_utils.h"

#include <algorithm>

namespace ck3eu5::ck3 {

namespace {
std::string getOptionalString(const common::PdsNode& node, std::string_view key_a, std::string_view key_b = "")
{
	if (const auto value = node.getString(key_a); !value.empty())
	{
		return value;
	}
	if (!key_b.empty())
	{
		return node.getString(key_b);
	}
	return {};
}
}  // namespace

std::string WorldImporter::deriveDisplayName(const std::string& key)
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

void WorldImporter::parseDynasties(const common::PdsNode& root, World& world)
{
	const auto* dynasties = root.get("dynasties");
	if (!dynasties)
	{
		return;
	}

	for (const auto& [key, value]: dynasties->properties())
	{
		Dynasty dynasty;
		dynasty.id = key;
		dynasty.key = getOptionalString(value, "key");
		if (dynasty.key.empty())
		{
			dynasty.key = key;
		}
		dynasty.display_name = getOptionalString(value, "display_name", "name");
		if (dynasty.display_name.empty())
		{
			dynasty.display_name = deriveDisplayName(dynasty.key);
		}
		dynasty.good_for_realm_name = value.getBool("good_for_realm_name", false);
		world.dynasties[key] = std::move(dynasty);
	}
}

void WorldImporter::parseDynastyHouses(const common::PdsNode& root, World& world)
{
	const auto* dynasty_houses = root.get("dynasty_houses");
	if (!dynasty_houses)
	{
		return;
	}

	for (const auto& [key, value]: dynasty_houses->properties())
	{
		DynastyHouse dynasty_house;
		dynasty_house.id = key;
		dynasty_house.key = getOptionalString(value, "key");
		dynasty_house.name = getOptionalString(value, "name", "display_name");
		dynasty_house.localized_name = getOptionalString(value, "localized_name");
		dynasty_house.prefix = getOptionalString(value, "prefix");
		dynasty_house.dynasty_id = getOptionalString(value, "dynasty", "dynasty_id");
		dynasty_house.house_head_id = getOptionalString(value, "house_head_id", "head_of_house");
		if (dynasty_house.name.empty())
		{
			dynasty_house.name = deriveDisplayName(key);
		}
		world.dynasty_houses[key] = std::move(dynasty_house);
	}
}

void WorldImporter::parseCultures(const common::PdsNode& root, World& world)
{
	const auto* cultures = root.get("cultures");
	if (!cultures)
	{
		return;
	}

	for (const auto& [key, value]: cultures->properties())
	{
		Culture culture;
		culture.id = key;
		culture.key = getOptionalString(value, "key", "template");
		if (culture.key.empty())
		{
			culture.key = key;
		}
		culture.display_name = getOptionalString(value, "display_name", "name");
		if (culture.display_name.empty())
		{
			culture.display_name = deriveDisplayName(culture.key);
		}
		culture.ethos = getOptionalString(value, "ethos");
		culture.heritage = getOptionalString(value, "heritage");
		culture.language = getOptionalString(value, "language");
		culture.parents = value.getListOfScalars("parents");
		world.cultures[key] = std::move(culture);
	}
}

void WorldImporter::parseFaiths(const common::PdsNode& root, World& world)
{
	const auto* faiths = root.get("faiths");
	if (!faiths)
	{
		return;
	}

	for (const auto& [key, value]: faiths->properties())
	{
		Faith faith;
		faith.id = key;
		faith.key = getOptionalString(value, "key", "template");
		if (faith.key.empty())
		{
			faith.key = key;
		}
		faith.religion = getOptionalString(value, "religion");
		faith.display_name = getOptionalString(value, "display_name", "name");
		if (faith.display_name.empty())
		{
			faith.display_name = deriveDisplayName(faith.key);
		}
		faith.religion_display_name = getOptionalString(value, "religion_display_name");
		faith.religion_family = getOptionalString(value, "religion_family");
		faith.doctrines = value.getListOfScalars("doctrines");
		world.faiths[key] = std::move(faith);
	}
}

void WorldImporter::parseCharacters(const common::PdsNode& root, World& world)
{
	const auto* characters = root.get("characters");
	if (!characters)
	{
		return;
	}

	for (const auto& [key, value]: characters->properties())
	{
		Character character;
		character.id = key;
		character.first_name = getOptionalString(value, "first_name", "name");
		character.last_name = getOptionalString(value, "last_name");
		character.dynasty = getOptionalString(value, "dynasty");
		character.dynasty_house_id = getOptionalString(value, "dynasty_house_id");
		character.culture = getOptionalString(value, "culture");
		character.culture_id = getOptionalString(value, "culture_id");
		character.faith = getOptionalString(value, "faith", "religion");
		character.faith_id = getOptionalString(value, "faith_id");
		character.government = getOptionalString(value, "government");
		character.primary_title = getOptionalString(value, "primary_title");
		character.liege = getOptionalString(value, "liege");
		character.employer_id = getOptionalString(value, "employer", "employer_id");
		character.spouse_id = getOptionalString(value, "spouse", "spouse_id");
		character.suzerain_id = getOptionalString(value, "suzerain", "suzerain_id");
		character.realm_capital_province = getOptionalString(value, "realm_capital_province");
		character.birth_date = getOptionalString(value, "birth_date");
		if (character.birth_date.empty())
		{
			character.birth_date = "1300.1.1";
		}
		character.death_date = getOptionalString(value, "death_date");
		character.gold = value.getDouble("gold", 0.0);
		character.realm_current_strength = value.getDouble("realm_current_strength", 0.0);
		character.realm_max_strength = value.getDouble("realm_max_strength", 0.0);
		character.realm_levy = value.getDouble("realm_levy", 0.0);
		character.adm = value.getInt("adm", 50);
		character.dip = value.getInt("dip", 50);
		character.mil = value.getInt("mil", 50);
		character.female = value.getBool("female", false);
		character.dead = value.getBool("dead", false) || !character.death_date.empty();
		character.claims = value.getListOfScalars("claims");
		character.domain_titles = value.getListOfScalars("domain_titles");
		world.characters[key] = std::move(character);
	}
}

void WorldImporter::parseTitles(const common::PdsNode& root, World& world)
{
	const auto* titles = root.get("titles");
	if (!titles)
	{
		return;
	}

	for (const auto& [key, value]: titles->properties())
	{
		Title title;
		title.key = key;
		title.source_id = getOptionalString(value, "source_id");
		title.rank = titleRankFromString(value.getString("rank"));
		if (title.rank == TitleRank::Unknown)
		{
			title.rank = titleRankFromTitleKey(key);
		}
		title.holder_id = getOptionalString(value, "holder", "holder_id");
		title.de_jure_liege_title = getOptionalString(value, "de_jure_liege_title", "liege_title");
		title.de_facto_liege_title = getOptionalString(value, "de_facto_liege_title", "de_facto_liege");
		title.government = getOptionalString(value, "government");
		title.capital_county = getOptionalString(value, "capital_county");
		title.capital_province = getOptionalString(value, "capital_province");
		title.display_name = getOptionalString(value, "display_name", "name");
		title.de_jure_vassals = value.getListOfScalars("de_jure_vassals");
		title.de_facto_vassals = value.getListOfScalars("de_facto_vassals");
		title.owned_de_jure_counties = value.getListOfScalars("owned_de_jure_counties");
		title.owned_de_facto_counties = value.getListOfScalars("owned_de_facto_counties");
		title.heirs = value.getListOfScalars("heirs");
		title.claimants = value.getListOfScalars("claimants");
		title.electors = value.getListOfScalars("electors");
		title.previous_holders = value.getListOfScalars("previous_holders");
		title.capital_barony = value.getBool("capital_barony", false);
		if (title.display_name.empty())
		{
			title.display_name = deriveDisplayName(key);
		}
		world.titles[key] = std::move(title);
	}
}

void WorldImporter::parseWars(const common::PdsNode& root, World& world)
{
	const auto* wars = root.get("wars");
	if (!wars)
	{
		return;
	}

	for (const auto& [key, value]: wars->properties())
	{
		War war;
		war.id = key;
		war.name = getOptionalString(value, "name");
		war.cb_type = getOptionalString(value, "cb_type");
		war.start_date = getOptionalString(value, "start_date");
		war.claimant_id = getOptionalString(value, "claimant");
		war.targeted_titles = value.getListOfScalars("targeted_titles");

		auto parse_side = [](const std::vector<const common::PdsNode*>& side_nodes,
						   std::string& leader_id,
						   std::vector<WarParticipant>& participants) {
			for (const auto* side_node: side_nodes)
			{
				if (!side_node)
				{
					continue;
				}
				if (side_node->isScalar())
				{
					if (leader_id.empty())
					{
						leader_id = side_node->scalar();
					}
					continue;
				}

				const auto* participant_list = side_node->get("participants");
				if (!participant_list)
				{
					continue;
				}
				for (const auto& participant_node: participant_list->items())
				{
					WarParticipant participant;
					participant.character_id = participant_node.getString("character");
					participant.joined_date = participant_node.getString("date");
					participant.contribution_score = participant_node.getDouble("contribution_score", 0.0);
					if (participant.contribution_score <= 0.0)
					{
						if (const auto* contribution = participant_node.get("contribution"))
						{
							for (const auto& item: contribution->items())
							{
								participant.contribution_score += item.asDouble(0.0);
							}
						}
					}
					if (!participant.character_id.empty())
					{
						if (leader_id.empty())
						{
							leader_id = participant.character_id;
						}
						participants.push_back(std::move(participant));
					}
				}
			}
		};

		parse_side(value.getAll("attacker"), war.attacker_id, war.attackers);
		parse_side(value.getAll("defender"), war.defender_id, war.defenders);
		world.wars[key] = std::move(war);
	}
}

void WorldImporter::parseCounties(const common::PdsNode& root, World& world)
{
	const auto* counties = root.get("counties");
	if (!counties)
	{
		return;
	}

	for (const auto& [key, value]: counties->properties())
	{
		County county;
		county.key = key;
		county.source_title_id = getOptionalString(value, "source_title_id");
		county.owner_id = getOptionalString(value, "owner", "holder");
		county.top_liege_id = getOptionalString(value, "top_liege");
		county.culture = getOptionalString(value, "culture");
		county.culture_id = getOptionalString(value, "culture_id");
		county.faith = getOptionalString(value, "faith", "religion");
		county.faith_id = getOptionalString(value, "faith_id");
		county.government = getOptionalString(value, "government");
		county.province_key = getOptionalString(value, "province", "province_key");
		county.display_name = getOptionalString(value, "display_name", "name");
		if (county.display_name.empty())
		{
			county.display_name = deriveDisplayName(key);
		}
		county.terrain = getOptionalString(value, "terrain");
		county.development = value.getInt("development", 1);
		county.holdings = value.getListOfScalars("holdings");
		county.barony_keys = value.getListOfScalars("baronies");
		county.barony_display_names = value.getListOfScalars("barony_display_names");
		county.barony_province_keys = value.getListOfScalars("barony_provinces");
		county.neighbors = value.getListOfScalars("neighbors");
		world.counties[key] = std::move(county);
	}
}

void WorldImporter::synthesizeCountiesFromTitles(World& world)
{
	for (const auto& [title_key, title]: world.titles)
	{
		if (title.rank != TitleRank::County)
		{
			continue;
		}
		if (world.counties.contains(title_key))
		{
			continue;
		}
		if (titleRankFromTitleKey(title_key) != TitleRank::County)
		{
			continue;
		}
		// Frontier-office titles (c_nf_*) appear in saves as county-rank titles but do not
		// carry actual county/barony geometry. Treat them as non-spatial titles instead of
		// synthesizing fake map counties from them.
		if (title_key.rfind("c_nf_", 0) == 0)
		{
			continue;
		}
		// Skip titular or administrative county-rank titles that point at another county for their capital.
		if (!title.capital_county.empty() && title.capital_county != title_key)
		{
			continue;
		}
		County county;
		county.key = title_key;
		county.source_title_id = title.source_id;
		county.owner_id = title.holder_id;
		county.government = title.government;
		county.province_key = common::sanitizeIdentifier(title.display_name) + "_province";
		county.display_name = title.display_name;
		county.development = 5;
		county.holdings = {"castle"};
		world.counties[title_key] = std::move(county);
	}
}

void WorldImporter::backfillRelationships(World& world)
{
	for (auto& [county_key, county]: world.counties)
	{
		(void)county_key;
		if (county.culture.empty() && !county.culture_id.empty())
		{
			if (const auto* culture = world.getCulture(county.culture_id))
			{
				county.culture = culture->key;
			}
		}
		if (county.faith.empty() && !county.faith_id.empty())
		{
			if (const auto* faith = world.getFaith(county.faith_id))
			{
				county.faith = faith->key;
			}
		}
	}

	for (auto& [character_id, character]: world.characters)
	{
		(void)character_id;
		if (character.dynasty.empty() && !character.dynasty_house_id.empty())
		{
			if (const auto* dynasty_house = world.getDynastyHouse(character.dynasty_house_id))
			{
				character.dynasty = dynasty_house->name;
			}
		}
		if (character.culture.empty() && !character.culture_id.empty())
		{
			if (const auto* culture = world.getCulture(character.culture_id))
			{
				character.culture = culture->key;
			}
		}
		if (character.faith.empty() && !character.faith_id.empty())
		{
			if (const auto* faith = world.getFaith(character.faith_id))
			{
				character.faith = faith->key;
			}
		}
		if (character.government.empty() && !character.primary_title.empty())
		{
			if (const auto* title = world.getTitle(character.primary_title))
			{
				character.government = title->government;
			}
		}
	}

	for (auto& [house_id, dynasty_house]: world.dynasty_houses)
	{
		if (dynasty_house.name.empty())
		{
			if (!dynasty_house.localized_name.empty())
			{
				dynasty_house.name = dynasty_house.localized_name;
			}
			else if (const auto* dynasty = world.getDynasty(dynasty_house.dynasty_id))
			{
				dynasty_house.name = dynasty->display_name;
			}
			else
			{
				dynasty_house.name = deriveDisplayName(house_id);
			}
		}
	}

	RealmNormalizer normalizer;
	normalizer.normalize(world);
}

World WorldImporter::importText(std::string_view text, diagnostics::DiagnosticsReport& diagnostics) const
{
	common::PdsParser parser;
	const auto parsed_root = parser.parse(text);
	common::PdsNode root = parsed_root;
	if (const auto* wrapped = parsed_root.get("world"))
	{
		root = *wrapped;
	}

	World world;
	world.date = root.getString("date", "1337.1.1");
	parseDynasties(root, world);
	parseDynastyHouses(root, world);
	parseCultures(root, world);
	parseFaiths(root, world);
	parseCharacters(root, world);
	parseTitles(root, world);
	parseWars(root, world);
	parseCounties(root, world);
	synthesizeCountiesFromTitles(world);
	backfillRelationships(world);

	if (world.counties.empty())
	{
		diagnostics.error("CK3_IMPORT_NO_COUNTIES", "No counties were found in the input world.");
	}
	if (world.characters.empty())
	{
		diagnostics.warning("CK3_IMPORT_NO_CHARACTERS", "No characters were found in the input world.");
	}
	return world;
}

World WorldImporter::importFromConfiguration(const config::Configuration& configuration, diagnostics::DiagnosticsReport& diagnostics) const
{
	common::Logger::info("Reading CK3 input...");
	InputReader reader;
	const auto text = reader.read(configuration);
	common::Logger::info("Parsing CK3 world snapshot...");
	auto world = importText(text, diagnostics);

	if (!configuration.ck3_game_path.empty())
	{
		common::Logger::info("Loading installed CK3 metadata...");
		InstalledTitlesLoader installed_titles_loader;
		const auto installed_data = installed_titles_loader.load(configuration.ck3_game_path);
		common::Logger::info("Enriching CK3 world from installed data...");
		InstalledWorldEnricher enricher;
		enricher.enrich(world, installed_data);
		backfillRelationships(world);
	}

	return world;
}

}  // namespace ck3eu5::ck3
