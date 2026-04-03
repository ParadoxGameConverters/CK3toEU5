#include "ck3/world_importer.h"
#include "ck3/installed_titles.h"
#include "ck3/raw_save_normalizer.h"
#include "common/filesystem_utils.h"
#include "config/configuration_loader.h"
#include "convert/world_converter.h"
#include "diagnostics/diagnostics_report.h"
#include "eu5/framework_builder.h"
#include "eu5/installed_data_extractor.h"
#include "eu5/installed_definitions_loader.h"
#include "eu5/world_sanitizer.h"
#include "eu5/world_validator.h"
#include "mappers/bootstrap_province_mapping_generator.h"
#include "mappers/mapper_bundle.h"
#include "mappers/province_matcher.h"
#include "output/eu5_outputter.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace {

void runEu5ExtractorSmokeCheck()
{
	const fs::path temp_root = fs::path(__FILE__).parent_path() / "generated_extractor_smoke";
	std::error_code ec;
	fs::remove_all(temp_root, ec);

	ck3eu5::common::writeTextFile(temp_root / "in_game/map_data/named_locations/00_default.txt", "london = abc123\noxford = def456\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/map_data/definitions.txt",
		 R"(europe = {
	channel_region = {
		thames_area = {
			london_province = { london }
			oxford_province = { oxford }
		}
	}
})");
	ck3eu5::common::writeTextFile(temp_root / "in_game/map_data/location_templates.txt",
		 R"(europe = {
	channel_region = {
		thames_area = {
			london_province = { london }
			oxford_province = { oxford }
		}
	}
}
london = { topography = flatland climate = oceanic raw_material = cloth natural_harbor_suitability = 0.75 }
oxford = { topography = flatland climate = oceanic raw_material = grain }
)");
	ck3eu5::common::writeTextFile(temp_root / "in_game/map_data/ports.csv",
		 "LandProvince;SeaZone;x;y;\nlondon;channel;1;2;x\n");
	ck3eu5::common::writeTextFile(temp_root / "main_menu/setup/start/07_cities_and_buildings.txt",
		 R"(locations = {
	london = { rank = city town_setup = british_town_port }
	oxford = { rank = town town_setup = british_town }
})");
	ck3eu5::common::writeTextFile(temp_root / "main_menu/common/named_colors/02_map.txt",
		 R"(colors = {
	map_FRA = rgb { 1 2 3 }
})");
	ck3eu5::common::writeTextFile(temp_root / "in_game/setup/countries/france.txt",
		 R"(FRA = {
	color = map_FRA
	color2 = rgb { 4 5 6 }
	unit_color0 = rgb { 7 8 9 }
	description_category = military
	difficulty = 2
})");

	ck3eu5::diagnostics::DiagnosticsReport diagnostics;
	ck3eu5::eu5::InstalledDataExtractor extractor;
	const auto framework = extractor.extract(temp_root, diagnostics);
	assert(!diagnostics.hasErrors());
	assert(framework.locations.size() == 2);
	assert(framework.colors.size() == 1);
	assert(framework.locations.at("london").province_definition == "london_province");
	assert(framework.locations.at("london").region == "channel_region");
	assert(framework.locations.at("london").area == "thames_area");
	assert(framework.locations.at("london").default_rank == "city");
	assert(framework.locations.at("london").town_setup == "british_town_port");
	assert(framework.locations.at("london").coastal);
	assert(framework.locations.at("london").raw_good == "cloth");
	assert(framework.colors.at("FRA").color == "rgb { 1 2 3 }");
	assert(framework.colors.at("FRA").color2 == "rgb { 4 5 6 }");

	const fs::path output_directory = temp_root / "output";
	extractor.writeCsvs(framework, output_directory / "location_framework.csv", output_directory / "country_colors.csv");
	assert(fs::exists(output_directory / "location_framework.csv"));
	assert(fs::exists(output_directory / "country_colors.csv"));

	fs::remove_all(temp_root, ec);
}

void runRawNormalizerSmokeCheck()
{
	const std::string raw_save = R"(date=1066.9.15
culture_manager={
	cultures={
		27={ culture_template="anglo_saxon" }
	}
}
religion={
	religions={
		4={ tag="christianity_religion" faiths={ 10 } }
	}
	faiths={
		10={ template="catholic" tag="catholic" religion=4 }
	}
}
dynasties={
	dynasties={
		900={ key="dynasty_wessex" good_for_realm_name=yes }
	}
	dynasty_house={
		6054={ key="house_wessex" name="dynn_Wessex" localized_name="Wessex" dynasty=900 head_of_house=1 }
	}
}
landed_titles={
	landed_titles={
		1={
			key="k_england"
			holder=1
			name="England"
			capital=100
			history_government="feudal_government"
			de_jure_vassals={ 5 }
		}
		5={
			key="d_essex"
			holder=1
			de_facto_liege=1
			name="Essex"
			capital=100
			history_government="feudal_government"
			de_jure_vassals={ 2 }
		}
		2={
			key="c_middlesex"
			holder=1
			de_facto_liege=5
			name="Middlesex"
			capital=100
			history_government="feudal_government"
			de_jure_vassals={ 3 4 }
			heir={ 2 }
			claim={ 3 }
			succession_election={
				electors={ 1 2 3 }
			}
			history={
				1050.1.1=4
				1048.1.1=2
			}
		}
		3={
			key="b_london"
			holder=1
			de_facto_liege=2
			name="London"
			capital=100
			capital_barony=yes
		}
		4={
			key="b_westminster"
			holder=3
			de_facto_liege=2
			name="Westminster"
			capital=100
			theocratic_lease=yes
		}
	}
}
living={
	1={
		first_name="Edward"
		birth=1040.1.1
		culture=27
		faith=10
		dynasty_house=6054
		family_data={
			primary_spouse=2
		}
		skill={ 8 6 9 4 5 3 }
		alive_data={
			gold=200
			landed_data={
				domain={ 1 5 2 }
				government="feudal_government"
				realm_capital=100
				current_strength=6400
				strength=7100
				levy=5200
			}
		}
	}
	2={
		first_name="Edith"
		birth=1042.1.1
		culture=27
		faith=10
		dynasty_house=6054
		family_data={
			primary_spouse=1
		}
		skill={ 5 4 4 4 5 5 }
		alive_data={
			gold=50
		}
	}
	3={
		first_name="Oswald"
		birth=1035.1.1
		culture=27
		faith=10
		skill={ 2 2 2 2 2 2 }
		alive_data={
			claim={
				title=5
			}
			court_data={
				employer=1
			}
			obedience_target=1
			landed_data={
				domain={ 4 }
				government="theocracy_government"
				realm_capital=100
			}
		}
	}
}
dead_unprunable={
	4={
		first_name="Aethelred"
		birth=1008.1.1
		culture=27
		faith=10
		dynasty_house=6054
		skill={ 3 4 5 2 4 3 }
		dead_data={
			date=1050.1.1
		}
	}
}
wars={
	active_wars={
		77={
			name="Essex Claim"
			start_date=1066.9.15
			casus_belli={
				type="claimant_war"
				attacker=3
				defender=1
				claimant=3
				targeted_titles={ 2 }
			}
			attacker={
				participants={
					{
						character=3
						date=1066.9.15
						contribution={ 12 4 }
					}
				}
			}
			defender={
				participants={
					{
						character=1
						date=1066.9.15
						contribution={ 16 2 }
					}
				}
			}
		}
	}
}
county_manager={
	counties={
		c_middlesex={
			development=7
			culture=27
			faith=10
		}
	}
}
)";

	ck3eu5::diagnostics::DiagnosticsReport diagnostics;
	ck3eu5::ck3::WorldImporter importer;
	const auto normalized = ck3eu5::ck3::normalizeMeltedSave(raw_save);
	const auto world = importer.importText(normalized, diagnostics);

	assert(world.characters.size() == 4);
	assert(world.dynasties.size() == 1);
	assert(world.dynasty_houses.size() == 1);
	assert(world.cultures.size() == 1);
	assert(world.faiths.size() == 1);
	assert(world.titles.contains("k_england"));
	assert(world.titles.contains("d_essex"));
	assert(world.counties.contains("c_middlesex"));
	assert(world.dynasties.at("900").key == "dynasty_wessex");
	assert(world.dynasties.at("900").good_for_realm_name);
	assert(world.dynasty_houses.at("6054").name == "Wessex");
	assert(world.dynasty_houses.at("6054").key == "house_wessex");
	assert(world.dynasty_houses.at("6054").localized_name == "Wessex");
	assert(world.dynasty_houses.at("6054").dynasty_id == "900");
	assert(world.dynasty_houses.at("6054").house_head_id == "1");
	assert(world.cultures.at("27").key == "anglo_saxon");
	assert(world.faiths.at("10").key == "catholic");
	assert(world.faiths.at("10").religion == "christianity_religion");
	assert(world.characters.at("1").primary_title == "k_england");
	assert(world.characters.at("1").dynasty_house_id == "6054");
	assert(world.characters.at("1").culture_id == "27");
	assert(world.characters.at("1").faith_id == "10");
	assert(world.characters.at("1").realm_capital_province == "100");
	assert(world.characters.at("1").realm_current_strength == 6400);
	assert(world.characters.at("1").realm_max_strength == 7100);
	assert(world.characters.at("1").realm_levy == 5200);
	assert(world.characters.at("1").spouse_id == "2");
	assert(std::find(world.characters.at("1").domain_titles.begin(),
				 world.characters.at("1").domain_titles.end(),
				 "c_middlesex") != world.characters.at("1").domain_titles.end());
	assert(std::find(world.characters.at("1").held_titles.begin(),
				 world.characters.at("1").held_titles.end(),
				 "k_england") != world.characters.at("1").held_titles.end());
	assert(world.characters.at("2").spouse_id == "1");
	assert(world.characters.at("3").employer_id == "1");
	assert(world.characters.at("3").suzerain_id == "1");
	assert(world.characters.at("3").claims.size() == 2);
	assert(std::find(world.characters.at("3").claims.begin(),
				 world.characters.at("3").claims.end(),
				 "d_essex") != world.characters.at("3").claims.end());
	assert(std::find(world.characters.at("3").claims.begin(),
				 world.characters.at("3").claims.end(),
				 "c_middlesex") != world.characters.at("3").claims.end());
	assert(world.characters.at("4").dead);
	assert(world.characters.at("4").death_date == "1050.1.1");
	assert(world.counties.at("c_middlesex").faith == "catholic");
	assert(world.counties.at("c_middlesex").culture == "anglo_saxon");
	assert(world.titles.at("c_middlesex").source_id == "2");
	assert(world.titles.at("c_middlesex").capital_province == "100");
	assert(world.titles.at("c_middlesex").de_jure_vassals.size() == 2);
	assert(world.titles.at("c_middlesex").heirs.size() == 1);
	assert(world.titles.at("c_middlesex").heirs.front() == "2");
	assert(world.titles.at("c_middlesex").claimants.size() == 1);
	assert(world.titles.at("c_middlesex").claimants.front() == "3");
	assert(world.titles.at("c_middlesex").electors.size() == 3);
	assert(world.titles.at("c_middlesex").previous_holders.size() == 2);
	assert(world.titles.at("c_middlesex").previous_holders.front() == "4");
	assert(world.titles.at("k_england").de_jure_vassals.size() == 1);
	assert(world.titles.at("k_england").de_jure_vassals.front() == "d_essex");
	assert(world.titles.at("k_england").de_facto_vassals.size() == 1);
	assert(world.titles.at("k_england").de_facto_vassals.front() == "d_essex");
	assert(world.titles.at("d_essex").de_facto_vassals.size() == 1);
	assert(world.titles.at("d_essex").de_facto_vassals.front() == "c_middlesex");
	assert(world.deJureCountyKeysOfTitle("k_england").size() == 1);
	assert(world.deJureCountyKeysOfTitle("k_england").front() == "c_middlesex");
	assert(world.deFactoCountyKeysOfTitle("k_england").size() == 1);
	assert(world.deFactoCountyKeysOfTitle("k_england").front() == "c_middlesex");
	assert(world.getTitleBySourceId("5"));
	assert(world.getTitleBySourceId("5")->key == "d_essex");
	assert(world.counties.at("c_middlesex").source_title_id == "2");
	assert(world.counties.at("c_middlesex").holdings.size() >= 3);
	assert(world.counties.at("c_middlesex").barony_keys.size() == 2);
	assert(world.counties.at("c_middlesex").barony_keys.front() == "b_london");
	assert(world.counties.at("c_middlesex").barony_display_names.front() == "London");
	assert(world.counties.at("c_middlesex").barony_province_keys.size() == 2);
	assert(world.counties.at("c_middlesex").barony_province_keys.front() == "100");
	assert(world.wars.size() == 1);
	assert(world.wars.contains("77"));
	assert(world.wars.at("77").cb_type == "claimant_war");
	assert(world.wars.at("77").attacker_id == "3");
	assert(world.wars.at("77").defender_id == "1");
	assert(world.wars.at("77").claimant_id == "3");
	assert(world.wars.at("77").targeted_titles.size() == 1);
	assert(world.wars.at("77").targeted_titles.front() == "c_middlesex");
	assert(world.wars.at("77").attackers.size() == 1);
	assert(world.wars.at("77").defenders.size() == 1);
	assert(world.wars.at("77").attackers.front().character_id == "3");
	assert(world.wars.at("77").attackers.front().contribution_score == 16);
	assert(world.wars.at("77").defenders.front().character_id == "1");
	assert(world.wars.at("77").defenders.front().contribution_score == 18);
}

void runProvinceMatcherSmokeCheck()
{
	ck3eu5::eu5::WorldFramework framework;
	framework.locations["aachen"] = {.key = "aachen", .display_name = "Aachen"};
	framework.locations["london"] = {.key = "london", .province_definition = "middlesex_province", .display_name = "London"};
	framework.locations["westminster"] = {
		 .key = "westminster", .province_definition = "middlesex_province", .display_name = "Westminster"};
	framework.locations["oxford"] = {.key = "oxford", .display_name = "Oxford"};
	framework.locations["fengzhou"] = {.key = "fengzhou", .display_name = "Fengzhou"};
	framework.province_to_locations["middlesex_province"] = {"london", "westminster"};

	ck3eu5::mappers::ProvinceMatcher matcher(framework);

	ck3eu5::ck3::County aachen;
	aachen.key = "c_aachen";
	aachen.display_name = "Aachen";
	const auto aachen_match = matcher.match(aachen);
	assert(aachen_match.has_value());
	assert(aachen_match->source == "exact_location_key");
	assert(aachen_match->eu5_locations.size() == 1);
	assert(aachen_match->eu5_locations.front() == "aachen");

	ck3eu5::ck3::County middlesex;
	middlesex.key = "c_middlesex";
	middlesex.display_name = "Middlesex";
	const auto middlesex_match = matcher.match(middlesex);
	assert(middlesex_match.has_value());
	assert(middlesex_match->source == "exact_province_definition");
	assert(middlesex_match->eu5_locations.size() == 2);

	ck3eu5::ck3::County westminster;
	westminster.key = "c_unknown";
	westminster.display_name = "Westminster";
	const auto westminster_match = matcher.match(westminster);
	assert(westminster_match.has_value());
	assert(westminster_match->source == "exact_display_name");
	assert(westminster_match->eu5_locations.size() == 1);
	assert(westminster_match->eu5_locations.front() == "westminster");

	ck3eu5::ck3::County oxfordshire;
	oxfordshire.key = "c_unknown";
	oxfordshire.display_name = "Unknown";
	oxfordshire.barony_keys = {"b_oxford"};
	oxfordshire.barony_display_names = {"Oxford"};
	const auto oxfordshire_match = matcher.match(oxfordshire);
	assert(oxfordshire_match.has_value());
	assert(oxfordshire_match->source == "barony_exact_location_key");
	assert(oxfordshire_match->eu5_locations.size() == 1);
	assert(oxfordshire_match->eu5_locations.front() == "oxford");

	ck3eu5::ck3::County synthetic_county;
	synthetic_county.key = "c_unknown";
	synthetic_county.display_name = "Unknown";
	synthetic_county.barony_keys = {"b_Liao_Fengzhou4"};
	const auto synthetic_match = matcher.match(synthetic_county);
	assert(synthetic_match.has_value());
	assert(synthetic_match->source == "barony_exact_location_key");
	assert(synthetic_match->eu5_locations.size() == 1);
	assert(synthetic_match->eu5_locations.front() == "fengzhou");
}

void runInstalledDefinitionsValidationSmokeCheck()
{
	const fs::path temp_root = fs::path(__FILE__).parent_path() / "generated_validation_smoke";
	std::error_code ec;
	fs::remove_all(temp_root, ec);

	ck3eu5::common::writeTextFile(temp_root / "in_game/common/cultures/test.txt", "english = { }\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/common/religions/test.txt", "catholic = { }\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/common/government_types/test.txt", "monarchy = { }\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/common/country_ranks/test.txt", "rank_kingdom = { }\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/common/pop_types/test.txt",
		 "peasants = { }\nnobles = { }\nclergy = { }\nburghers = { }\nlaborers = { }\nsoldiers = { }\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/common/unit_types/test.txt",
		 "a_footmen = { }\nn_barque = { }\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/common/heir_selections/test.txt",
		 "cognatic_primogeniture = { }\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/common/laws/test.txt",
		 "marriage_law = { monogamous_marriage = { } }\n"
		 "heir_religion_law = { heir_same_religion = { } }\n"
		 "feudal_de_jure_law = { by_tradition = { } }\n"
		 "medieval_levy_law = { noble_levies = { } }\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/common/government_reforms/test.txt",
		 "feudal_nobility = { }\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/common/estate_privileges/test.txt",
		 "nobles_land_rights = { }\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/common/subject_types/test.txt",
		 "vassal = { }\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/common/building_types/test.txt",
		 "local_markets = { }\ncastle = { }\nbarracks = { }\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/common/scripted_relations/test.txt",
		 "alliance = { }\nguarantee = { }\n");
	ck3eu5::common::writeTextFile(temp_root / "in_game/common/subject_military_stances/test.txt",
		 "normal_military_stance = { }\n");

	ck3eu5::diagnostics::DiagnosticsReport diagnostics;
	ck3eu5::eu5::InstalledDefinitionsLoader loader;
	const auto definitions = loader.load(temp_root, diagnostics);
	assert(definitions.cultures.contains("english"));
	assert(definitions.religions.contains("catholic"));
	assert(definitions.government_types.contains("monarchy"));
	assert(definitions.country_ranks.contains("rank_kingdom"));
	assert(definitions.pop_types.contains("peasants"));
	assert(definitions.unit_types.contains("a_footmen"));
	assert(definitions.heir_selections.contains("cognatic_primogeniture"));
	assert(definitions.law_values.contains("monogamous_marriage"));
	assert(definitions.government_reforms.contains("feudal_nobility"));
	assert(definitions.estate_privileges.contains("nobles_land_rights"));
	assert(definitions.subject_types.contains("vassal"));
	assert(definitions.building_types.contains("local_markets"));
	assert(definitions.scripted_relations.contains("alliance"));
	assert(definitions.subject_military_stances.contains("normal_military_stance"));

	ck3eu5::eu5::World world;
	world.countries["ENG"] = {.tag = "ENG",
		 .primary_culture = "english",
		 .primary_religion = "catholic",
		 .government_type = "monarchy",
		 .country_rank = "rank_kingdom",
		 .heir_selection = "cognatic_primogeniture",
		 .laws = {{"marriage_law", "monogamous_marriage"},
					{"heir_religion_law", "heir_same_religion"},
					{"feudal_de_jure_law", "by_tradition"},
					{"medieval_levy_law", "noble_levies"}},
		 .reforms = {"feudal_nobility"},
		 .privileges = {"nobles_land_rights"}};
	world.locations["london"] = {.key = "london",
		 .culture = "english",
		 .religion = "catholic",
		 .pops = {ck3eu5::eu5::Pop{.type = "peasants", .culture = "english", .religion = "catholic", .size = 1.0}}};
	world.subject_relations.push_back({.liege_tag = "ENG", .subject_tag = "WLS", .subject_type = "vassal"});
	world.subject_relations.front().subject_military_stance = "normal_military_stance";
	world.scripted_relations.push_back({.first_tag = "ENG", .second_tag = "FRA", .type = "alliance", .mutual = true});
	world.buildings.push_back({.type = "local_markets", .location = "london", .tag = "ENG", .level = 1});
	world.start_forces.push_back(
		 {.key = "army_ENG",
			.tag = "ENG",
			.branch = "army",
			.location = "london",
			.commander_character_key = "char_1",
			.units = {ck3eu5::eu5::StartForceUnit{.type = "a_footmen", .count = 3}},
			.wartime = false});

	ck3eu5::eu5::WorldValidator validator;
	validator.validate(world, definitions, diagnostics);
	assert(!diagnostics.hasErrors());
	for (const auto& issue: diagnostics.issues())
	{
		assert(issue.code != "VALIDATION_UNKNOWN_CULTURE");
		assert(issue.code != "VALIDATION_UNKNOWN_RELIGION");
		assert(issue.code != "VALIDATION_UNKNOWN_GOVERNMENT");
		assert(issue.code != "VALIDATION_UNKNOWN_COUNTRY_RANK");
		assert(issue.code != "VALIDATION_UNKNOWN_POP_TYPE");
		assert(issue.code != "VALIDATION_UNKNOWN_HEIR_SELECTION");
		assert(issue.code != "VALIDATION_UNKNOWN_LAW");
		assert(issue.code != "VALIDATION_UNKNOWN_GOVERNMENT_REFORM");
		assert(issue.code != "VALIDATION_UNKNOWN_PRIVILEGE");
		assert(issue.code != "VALIDATION_UNKNOWN_SUBJECT_TYPE");
		assert(issue.code != "VALIDATION_UNKNOWN_BUILDING");
		assert(issue.code != "VALIDATION_UNKNOWN_SCRIPTED_RELATION");
		assert(issue.code != "VALIDATION_UNKNOWN_SUBJECT_STANCE");
		assert(issue.code != "VALIDATION_UNKNOWN_UNIT");
	}

	fs::remove_all(temp_root, ec);
}

void runWorldSanitizerSmokeCheck()
{
	ck3eu5::eu5::InstalledDefinitions definitions;
	definitions.cultures = {"english", "bedouin_culture", "east_franconian"};
	definitions.religions = {"catholic", "sunni"};
	definitions.government_types = {"monarchy"};
	definitions.country_ranks = {"rank_kingdom"};
	definitions.pop_types = {"peasants", "nobles"};
	definitions.heir_selections = {"cognatic_primogeniture"};
	definitions.law_values = {"monogamous_marriage"};
	definitions.government_reforms = {"feudal_nobility"};
	definitions.estate_privileges = {"nobles_land_rights"};
	definitions.subject_types = {"vassal"};
	definitions.building_types = {"dock", "coastal_fort", "castle"};
	definitions.unit_types = {"a_footmen", "n_barque"};
	definitions.scripted_relations = {"alliance"};
	definitions.subject_military_stances = {"normal_military_stance"};

	ck3eu5::eu5::WorldFramework framework;
	framework.locations["wasteland_capital"] = {.key = "wasteland_capital",
		 .display_name = "Wasteland Capital",
		 .topography = "mountain_wasteland",
		 .default_rank = "rural_settlement"};
	framework.locations["inland_town"] = {.key = "inland_town",
		 .display_name = "Inland Town",
		 .region = "europe_region",
		 .topography = "flatland",
		 .default_rank = "town"};
	framework.locations["port_city"] = {.key = "port_city",
		 .display_name = "Port City",
		 .region = "europe_region",
		 .topography = "flatland",
		 .default_rank = "city",
		 .coastal = true,
		 .has_port = true};
	framework.locations["shared_rural"] = {.key = "shared_rural",
		 .display_name = "Shared Rural",
		 .region = "europe_region",
		 .topography = "flatland",
		 .default_rank = "rural_settlement"};
	framework.locations["rogue_island"] = {.key = "rogue_island",
		 .display_name = "Rogue Island",
		 .region = "madagascar_region",
		 .topography = "flatland",
		 .default_rank = "rural_settlement"};
	for (int index = 1; index <= 6; ++index)
	{
		const auto key = "same_region_" + std::to_string(index);
		framework.locations[key] = {.key = key,
			 .display_name = key,
			 .region = "europe_region",
			 .topography = "flatland",
			 .default_rank = "rural_settlement"};
	}

	ck3eu5::eu5::World world;
	world.countries["ENG"] = {.tag = "ENG",
		 .capital_location = "wasteland_capital",
		 .primary_culture = "franconian",
		 .primary_religion = "ashari",
		 .government_type = "monarchy",
		 .country_rank = "rank_kingdom",
		 .ruler_character_key = "char_1",
		 .consort_character_key = "char_future",
		 .heir_character_key = "char_missing",
		 .heir_selection = "cognatic_primogeniture",
		 .laws = {{"marriage_law", "monogamous_marriage"}},
		 .reforms = {"feudal_nobility"},
		 .privileges = {"nobles_land_rights"},
		 .owned_core_locations = {"wasteland_capital",
				 "inland_town",
				 "port_city",
				 "shared_rural",
				 "rogue_island",
				 "same_region_1",
				 "same_region_2",
				 "same_region_3",
				 "same_region_4",
				 "same_region_5",
				 "same_region_6"}};
	world.countries["TMP"] = {.tag = "TMP",
		 .capital_location = "shared_rural",
		 .primary_culture = "generic",
		 .primary_religion = "generic_faith",
		 .government_type = "monarchy",
		 .country_rank = "rank_kingdom",
		 .heir_selection = "cognatic_primogeniture",
		 .owned_core_locations = {"shared_rural"}};
	world.locations["wasteland_capital"] = {.key = "wasteland_capital",
		 .owner_tag = "ENG",
		 .culture = "generic",
		 .religion = "generic_faith",
		 .rank = "rural_settlement",
		 .development = 3.0,
		 .pops = {ck3eu5::eu5::Pop{.type = "peasants", .culture = "generic", .religion = "generic_faith", .size = 1.0}}};
	world.locations["inland_town"] = {.key = "inland_town",
		 .owner_tag = "ENG",
		 .culture = "bedouin",
		 .religion = "ashari",
		 .rank = "town",
		 .region = "europe_region",
		 .development = 9.0,
		 .pops = {ck3eu5::eu5::Pop{.type = "peasants", .culture = "bedouin", .religion = "ashari", .size = 4.0}}};
	world.locations["port_city"] = {.key = "port_city",
		 .owner_tag = "ENG",
		 .culture = "bedouin",
		 .religion = "ashari",
		 .rank = "city",
		 .region = "europe_region",
		 .development = 12.0,
		 .pops = {ck3eu5::eu5::Pop{.type = "nobles", .culture = "franconian", .religion = "ashari", .size = 1.0},
					ck3eu5::eu5::Pop{.type = "peasants", .culture = "bedouin", .religion = "ashari", .size = 6.0}}};
	world.locations["shared_rural"] = {.key = "shared_rural",
		 .owner_tag = "ENG",
		 .culture = "bedouin",
		 .religion = "ashari",
		 .rank = "rural_settlement",
		 .region = "europe_region",
		 .development = 4.0,
		 .pops = {ck3eu5::eu5::Pop{.type = "peasants", .culture = "bedouin", .religion = "ashari", .size = 2.0}}};
	world.locations["rogue_island"] = {.key = "rogue_island",
		 .owner_tag = "ENG",
		 .culture = "bedouin",
		 .religion = "ashari",
		 .rank = "rural_settlement",
		 .region = "madagascar_region",
		 .development = 4.0,
		 .pops = {ck3eu5::eu5::Pop{.type = "peasants", .culture = "bedouin", .religion = "ashari", .size = 1.0}}};
	for (int index = 1; index <= 6; ++index)
	{
		const auto key = "same_region_" + std::to_string(index);
		world.locations[key] = {.key = key,
			 .owner_tag = "ENG",
			 .culture = "bedouin",
			 .religion = "ashari",
			 .rank = "rural_settlement",
			 .region = "europe_region",
			 .development = 2.0 + index,
			 .pops = {ck3eu5::eu5::Pop{.type = "peasants", .culture = "bedouin", .religion = "ashari", .size = 1.0}}};
	}
	world.characters["char_1"] = {.key = "char_1",
		 .culture = "generic",
		 .religion = "generic_faith",
		 .tag = "ENG"};
	world.characters["char_future"] = {.key = "char_future",
		 .culture = "generic",
		 .religion = "generic_faith",
		 .birth_date = "1400.1.1",
		 .tag = "ENG"};
	world.buildings.push_back({.type = "dock", .location = "inland_town", .tag = "ENG", .level = 1});
	world.start_forces.push_back({.key = "army_ENG",
		 .tag = "ENG",
		 .branch = "army",
		 .location = "wasteland_capital",
		 .commander_character_key = "char_1",
		 .units = {ck3eu5::eu5::StartForceUnit{.type = "a_footmen", .count = 2}},
		 .wartime = false});
	world.start_forces.push_back({.key = "navy_ENG",
		 .tag = "ENG",
		 .branch = "navy",
		 .location = "inland_town",
		 .units = {ck3eu5::eu5::StartForceUnit{.type = "n_barque", .count = 1}},
		 .wartime = false});
	world.force_plans.push_back({.key = "navy_ENG",
		 .tag = "ENG",
		 .branch = "navy",
		 .home_location = "inland_town"});

	ck3eu5::diagnostics::DiagnosticsReport diagnostics;
	ck3eu5::config::Configuration configuration;
	ck3eu5::eu5::WorldSanitizer sanitizer;
	sanitizer.sanitize(world, framework, configuration, definitions, diagnostics);

	assert(!world.countries.contains("TMP"));
	assert(world.countries.at("ENG").capital_location == "port_city");
	assert(world.countries.at("ENG").owned_core_locations.size() == 9);
	assert(world.countries.at("ENG").primary_culture == "bedouin_culture");
	assert(world.countries.at("ENG").primary_religion == "sunni");
	assert(world.countries.at("ENG").ruler_character_key == "char_1");
	assert(world.countries.at("ENG").consort_character_key.empty());
	assert(world.countries.at("ENG").heir_character_key.empty());
	assert(world.characters.at("char_1").culture == "bedouin_culture");
	assert(world.characters.at("char_1").religion == "sunni");
	assert(world.locations.at("inland_town").culture == "bedouin_culture");
	assert(world.locations.at("inland_town").religion == "sunni");
	assert(!world.locations.contains("wasteland_capital"));
	assert(!world.locations.contains("rogue_island"));
	assert(world.buildings.empty());
	assert(world.start_forces.size() == 2);
	assert(world.start_forces.front().location == "port_city");
	assert(world.start_forces.back().location == "port_city");
	assert(world.force_plans.front().home_location == "port_city");

	ck3eu5::eu5::WorldValidator validator;
	validator.validate(world, definitions, diagnostics);
	for (const auto& issue: diagnostics.issues())
	{
		assert(issue.code != "VALIDATION_UNKNOWN_CULTURE");
		assert(issue.code != "VALIDATION_UNKNOWN_RELIGION");
	}
}

void runBootstrapProvinceMappingSmokeCheck()
{
	const fs::path temp_root = fs::path(__FILE__).parent_path() / "generated_bootstrap_mapping_smoke";
	std::error_code ec;
	fs::remove_all(temp_root, ec);

	ck3eu5::common::writeTextFile(temp_root / "game/common/landed_titles/00_landed_titles.txt",
		 R"(e_england = {
	k_england = {
		d_essex = {
			c_middlesex = {
				b_london = { }
				b_westminster = { }
			}
			c_oxfordshire = {
				b_oxford = { }
			}
		}
	}
})");
	ck3eu5::common::writeTextFile(temp_root / "game/localization/english/titles_l_english.yml",
		 "l_english:\n"
		 "c_middlesex:0 \"Middlesex\"\n"
		 "b_london:0 \"London\"\n"
		 "b_westminster:0 \"Westminster\"\n"
		 "c_oxfordshire:0 \"Oxfordshire\"\n"
		 "b_oxford:0 \"Oxford\"\n",
		 ck3eu5::common::TextEncoding::Utf8Bom);

	ck3eu5::common::writeTextFile(temp_root / "existing_province_mappings.csv",
		 "ck3_county,eu5_locations\n"
		 "c_middlesex,london|westminster\n");

	ck3eu5::ck3::InstalledTitlesLoader loader;
	const auto installed_titles = loader.load(temp_root);
	assert(installed_titles.counties.size() == 2);
	assert(installed_titles.counties.at("c_middlesex").baronies.size() == 2);
	assert(installed_titles.counties.at("c_oxfordshire").baronies.front().display_name == "Oxford");

	ck3eu5::eu5::WorldFramework framework;
	framework.locations["london"] = {.key = "london", .display_name = "London"};
	framework.locations["westminster"] = {.key = "westminster", .display_name = "Westminster"};
	framework.locations["oxford"] = {.key = "oxford", .display_name = "Oxford"};

	ck3eu5::diagnostics::DiagnosticsReport diagnostics;
	ck3eu5::mappers::BootstrapProvinceMappingGenerator generator;
	const auto result = generator.generate(installed_titles, framework, temp_root / "existing_province_mappings.csv", diagnostics);

	assert(result.total_counties == 2);
	assert(result.manual_counties == 1);
	assert(result.generated_counties == 1);
	assert(result.unmapped_counties == 0);

	const auto middlesex_it = std::find_if(result.mappings.begin(), result.mappings.end(), [](const auto& mapping) {
		return mapping.ck3_county == "c_middlesex";
	});
	assert(middlesex_it != result.mappings.end());
	assert(middlesex_it->sources.size() == 1);
	assert(middlesex_it->sources.front() == "existing_manual");
	assert(middlesex_it->eu5_locations.size() == 2);

	const auto oxfordshire_it = std::find_if(result.mappings.begin(), result.mappings.end(), [](const auto& mapping) {
		return mapping.ck3_county == "c_oxfordshire";
	});
	assert(oxfordshire_it != result.mappings.end());
	assert(!oxfordshire_it->sources.empty());
	assert(oxfordshire_it->sources.front() == "barony_exact_location_key");
	assert(oxfordshire_it->eu5_locations.size() == 1);
	assert(oxfordshire_it->eu5_locations.front() == "oxford");

	generator.writeCsvs(result, temp_root / "generated/province_mappings.csv", temp_root / "generated/province_mappings_report.csv");
	assert(fs::exists(temp_root / "generated/province_mappings.csv"));
	assert(fs::exists(temp_root / "generated/province_mappings_report.csv"));

	fs::remove_all(temp_root, ec);
}

void runInstalledCk3EnrichmentSmokeCheck()
{
	const fs::path temp_root = fs::path(__FILE__).parent_path() / "generated_ck3_enrichment_smoke";
	std::error_code ec;
	fs::remove_all(temp_root, ec);

	ck3eu5::common::writeTextFile(temp_root / "input_world.pds",
		 R"(date = 1066.9.15
dynasties = {
	900 = {
		key = dynasty_wessex
	}
}
dynasty_houses = {
	6054 = {
		key = house_wessex
		name = dynn_Wessex
		dynasty = 900
	}
}
cultures = {
	27 = {
		key = anglo_saxon
	}
}
faiths = {
	10 = {
		key = catholic
	}
}
characters = {
	1 = {
		first_name = "Edward"
		dynasty_house_id = 6054
		culture_id = 27
		faith_id = 10
		domain_titles = { c_middlesex }
	}
}
titles = {
	c_middlesex = {
		holder = 1
	}
	b_london = {
		holder = 1
	}
}
counties = {
	c_middlesex = {
		owner = 1
	}
}
)");
	ck3eu5::common::writeTextFile(temp_root / "game/common/landed_titles/00_landed_titles.txt",
		 R"(e_england = {
	k_england = {
		d_essex = {
			c_middlesex = {
				b_london = { province = 123 }
			}
		}
	}
})");
	ck3eu5::common::writeTextFile(temp_root / "game/common/culture/cultures/00_test.txt",
		 R"(anglo_saxon = {
	ethos = ethos_bureaucratic
	heritage = heritage_west_germanic
	language = language_anglic
	parents = { old_saxon }
})");
	ck3eu5::common::writeTextFile(temp_root / "game/common/religion/religions/00_test.txt",
		 R"(christianity_religion = {
	family = rf_abrahamic
	faiths = {
		catholic = {
			doctrine = tenet_communion
			doctrine = tenet_monasticism
		}
	}
})");
	ck3eu5::common::writeTextFile(temp_root / "game/localization/english/test_l_english.yml",
		 "l_english:\n"
		 "c_middlesex:0 \"Middlesex\"\n"
		 "b_london:0 \"London\"\n"
		 "anglo_saxon:0 \"Anglo-Saxon\"\n"
		 "catholic:0 \"Catholic\"\n"
		 "christianity_religion:0 \"Christianity\"\n"
		 "dynn_Wessex:0 \"Wessex\"\n",
		 ck3eu5::common::TextEncoding::Utf8Bom);
	ck3eu5::common::writeTextFile(temp_root / "config.cfg",
		 "ck3_input = input_world.pds\n"
		 "ck3_game_path = .\n"
		 "output_mod_path = generated/out\n"
		 "location_framework = ../../data/configurables/location_framework.csv\n"
		 "province_mappings = ../../data/configurables/province_mappings.csv\n"
		 "title_mappings = ../../data/configurables/title_mappings.csv\n"
		 "culture_mappings = ../../data/configurables/culture_mappings.csv\n"
		 "religion_mappings = ../../data/configurables/religion_mappings.csv\n"
		 "government_mappings = ../../data/configurables/government_mappings.csv\n"
		 "country_colors = ../../data/configurables/country_colors.csv\n");

	ck3eu5::config::ConfigurationLoader loader;
	const auto configuration = loader.load(temp_root / "config.cfg");
	ck3eu5::diagnostics::DiagnosticsReport diagnostics;
	ck3eu5::ck3::WorldImporter importer;
	const auto world = importer.importFromConfiguration(configuration, diagnostics);

	assert(!diagnostics.hasErrors());
	assert(world.dynasties.at("900").display_name == "Dynasty wessex");
	assert(world.dynasty_houses.at("6054").name == "Wessex");
	assert(world.cultures.at("27").display_name == "Anglo-Saxon");
	assert(world.cultures.at("27").heritage == "heritage_west_germanic");
	assert(world.cultures.at("27").language == "language_anglic");
	assert(world.cultures.at("27").ethos == "ethos_bureaucratic");
	assert(world.cultures.at("27").parents.size() == 1);
	assert(world.cultures.at("27").parents.front() == "old_saxon");
	assert(world.faiths.at("10").display_name == "Catholic");
	assert(world.faiths.at("10").religion == "christianity_religion");
	assert(world.faiths.at("10").religion_display_name == "Christianity");
	assert(world.faiths.at("10").religion_family == "rf_abrahamic");
	assert(world.faiths.at("10").doctrines.size() == 2);
	assert(world.titles.at("c_middlesex").display_name == "Middlesex");
	assert(world.titles.at("c_middlesex").de_jure_liege_title == "d_essex");
	assert(world.titles.at("b_london").capital_province == "123");
	assert(world.counties.at("c_middlesex").display_name == "Middlesex");
	assert(world.counties.at("c_middlesex").barony_keys.size() == 1);
	assert(world.counties.at("c_middlesex").barony_keys.front() == "b_london");
	assert(world.counties.at("c_middlesex").barony_display_names.front() == "London");
	assert(world.counties.at("c_middlesex").barony_province_keys.front() == "123");

	fs::remove_all(temp_root, ec);
}

void runRealmNormalizationSmokeCheck()
{
	const std::string normalized_world = R"(date = 1066.9.15
characters = {
	1 = {
		first_name = "William"
		domain_titles = { d_essex c_middlesex }
	}
	2 = {
		first_name = "Harold"
		domain_titles = { c_oxfordshire }
	}
	3 = {
		first_name = "Edgar"
		employer = 1
	}
	4 = {
		first_name = "Edward"
		dead = yes
		death_date = 1050.1.1
	}
}
titles = {
	d_essex = {
		holder = 1
		government = feudal_government
		de_jure_vassals = { c_middlesex c_oxfordshire }
	}
	c_middlesex = {
		holder = 1
		government = feudal_government
		de_jure_liege_title = d_essex
		de_facto_liege_title = d_essex
		de_jure_vassals = { b_london }
		heirs = { 2 2 999 }
		claimants = { 2 1 999 }
		electors = { 2 999 }
		previous_holders = { 4 4 1 }
	}
	c_oxfordshire = {
		holder = 2
		government = feudal_government
		de_jure_liege_title = d_essex
		de_facto_liege_title = d_essex
		de_jure_vassals = { b_oxford }
	}
	b_london = {
		holder = 1
		de_facto_liege_title = c_middlesex
		capital_province = 100
	}
	b_oxford = {
		holder = 2
		de_facto_liege_title = c_oxfordshire
		capital_province = 200
	}
}
counties = {
	c_middlesex = {
	}
	c_oxfordshire = {
	}
}
)";

	ck3eu5::diagnostics::DiagnosticsReport diagnostics;
	ck3eu5::ck3::WorldImporter importer;
	const auto world = importer.importText(normalized_world, diagnostics);

	assert(!diagnostics.hasErrors());
	assert(world.characters.at("1").primary_title == "d_essex");
	assert(world.characters.at("1").government == "feudal_government");
	assert(world.characters.at("2").primary_title == "c_oxfordshire");
	assert(world.characters.at("2").liege == "1");
	assert(world.characters.at("3").liege == "1");
	assert(world.titles.at("d_essex").de_facto_vassals.size() == 2);
	assert(world.titles.at("d_essex").capital_county == "c_middlesex");
	assert(world.titles.at("d_essex").capital_province == "100");
	assert(world.titles.at("c_middlesex").capital_county == "c_middlesex");
	assert(world.titles.at("c_middlesex").capital_province == "100");
	assert(world.titles.at("c_middlesex").heirs.size() == 1);
	assert(world.titles.at("c_middlesex").heirs.front() == "2");
	assert(world.titles.at("c_middlesex").claimants.size() == 1);
	assert(world.titles.at("c_middlesex").claimants.front() == "2");
	assert(world.titles.at("c_middlesex").electors.size() == 1);
	assert(world.titles.at("c_middlesex").electors.front() == "2");
	assert(world.titles.at("c_middlesex").previous_holders.size() == 1);
	assert(world.titles.at("c_middlesex").previous_holders.front() == "4");
	assert(world.characters.at("2").claims.size() == 1);
	assert(world.characters.at("2").claims.front() == "c_middlesex");
	assert(world.counties.at("c_middlesex").owner_id == "1");
	assert(world.counties.at("c_middlesex").top_liege_id == "1");
	assert(world.counties.at("c_middlesex").province_key == "100");
	assert(world.counties.at("c_middlesex").barony_keys.size() == 1);
	assert(world.counties.at("c_middlesex").barony_keys.front() == "b_london");
	assert(world.counties.at("c_middlesex").barony_province_keys.front() == "100");
	assert(world.counties.at("c_oxfordshire").owner_id == "2");
	assert(world.counties.at("c_oxfordshire").top_liege_id == "1");
}

}  // namespace

int main()
{
	runEu5ExtractorSmokeCheck();
	runRawNormalizerSmokeCheck();
	runProvinceMatcherSmokeCheck();
	runInstalledDefinitionsValidationSmokeCheck();
	runWorldSanitizerSmokeCheck();
	runBootstrapProvinceMappingSmokeCheck();
	runInstalledCk3EnrichmentSmokeCheck();
	runRealmNormalizationSmokeCheck();

	const fs::path repo_root = fs::path(__FILE__).parent_path().parent_path();
	const fs::path config_path = repo_root / "examples/sample_config.cfg";
	const fs::path output_path = repo_root / "tests/generated_smoke_mod";
	std::error_code ec;
	fs::remove_all(output_path, ec);

	ck3eu5::config::ConfigurationLoader loader;
	auto configuration = loader.load(config_path);
	configuration.output_mod_path = output_path;
	configuration.verbose_logging = false;

	ck3eu5::diagnostics::DiagnosticsReport diagnostics;

	ck3eu5::ck3::WorldImporter ck3_importer;
	auto ck3_world = ck3_importer.importFromConfiguration(configuration, diagnostics);
	assert(ck3_world.characters.size() == 4);
	assert(ck3_world.counties.size() == 4);
	assert(ck3_world.titles.at("d_normandy").heirs.size() == 1);
	assert(ck3_world.titles.at("d_normandy").heirs.front() == "3");
	assert(ck3_world.titles.at("d_normandy").previous_holders.size() == 2);
	assert(ck3_world.characters.at("4").dead);

	ck3eu5::eu5::WorldFrameworkBuilder framework_builder;
	auto framework = framework_builder.load(configuration, diagnostics);
	assert(framework.locations.size() == 4);

	ck3eu5::mappers::MapperBundleBuilder mapper_builder;
	auto mappers = mapper_builder.load(configuration, diagnostics);
	assert(mappers.title_mappings.size() == 2);

	ck3eu5::convert::WorldConverter converter;
	auto eu5_world = converter.convert(ck3_world, framework, mappers, configuration, diagnostics);
	assert(eu5_world.countries.size() == 2);
	assert(eu5_world.locations.size() == 4);
	assert(eu5_world.characters.size() == 4);
	assert(eu5_world.subject_relations.size() == 1);
	assert(eu5_world.countries.contains("ENG"));
	assert(eu5_world.countries.contains("NRM"));
	assert(!eu5_world.wars.empty());
	assert(!eu5_world.start_forces.empty());
	assert(eu5_world.countries.at("ENG").government_power >= 20);
	assert(eu5_world.countries.at("ENG").laws.contains("marriage_law"));
	assert(eu5_world.countries.at("ENG").laws.contains("heir_religion_law"));
	assert(!eu5_world.countries.at("ENG").heir_selection.empty());
	assert(!eu5_world.countries.at("ENG").reforms.empty());
	assert(!eu5_world.countries.at("ENG").privileges.empty());
	assert(eu5_world.countries.at("NRM").heir_character_key == "char_3_william");
	assert(eu5_world.countries.at("NRM").consort_character_key == "char_3_william");
	assert(!eu5_world.markets.empty());
	assert(!eu5_world.buildings.empty());
	assert(!eu5_world.force_plans.empty());
	assert(std::any_of(eu5_world.start_forces.begin(), eu5_world.start_forces.end(), [](const auto& force) {
		return force.branch == "army" && !force.wartime;
	}));
	assert(std::any_of(eu5_world.start_forces.begin(), eu5_world.start_forces.end(), [](const auto& force) {
		return force.wartime;
	}));
	assert(eu5_world.wars.front().civil_war);
	assert(!eu5_world.wars.front().attackers.empty());
	assert(!eu5_world.wars.front().defenders.empty());
	assert(eu5_world.subject_relations.front().subject_military_stance == "normal_military_stance" ||
			 !eu5_world.subject_relations.front().subject_military_stance.empty());
	assert(eu5_world.characters.contains("char_3_william"));
	assert(eu5_world.characters.at("char_3_william").tag == "NRM");
	assert(eu5_world.characters.contains("char_4_richard"));
	assert(eu5_world.characters.at("char_4_richard").tag == "NRM");
	assert(eu5_world.characters.at("char_4_richard").death_date == "1330.4.1");

	ck3eu5::output::Eu5Outputter outputter;
	outputter.write(eu5_world, framework, configuration, diagnostics);

	assert(fs::exists(output_path / ".metadata/metadata.json"));
	assert(fs::exists(output_path / "main_menu/setup/start/10_countries.txt"));
	assert(fs::exists(output_path / "main_menu/setup/start/13_religion.txt"));
	assert(fs::exists(output_path / "in_game/setup/countries/00_ck3_generated_countries.txt"));
	assert(fs::exists(output_path / "in_game/localization/english/ck3_to_eu5_l_english.yml"));
	assert(fs::exists(output_path / "debug/characters.csv"));
	assert(fs::exists(output_path / "main_menu/setup/start/07_cities_and_buildings.txt"));
	assert(fs::exists(output_path / "main_menu/setup/start/16_wars.txt"));
	assert(fs::exists(output_path / "in_game/common/on_action/zz_ck3eu5_startup.txt"));
	assert(fs::exists(output_path / "in_game/events/zz_ck3eu5_startup.txt"));
	assert(fs::exists(output_path / "debug/markets.csv"));
	assert(fs::exists(output_path / "debug/buildings.csv"));
	assert(fs::exists(output_path / "debug/force_plans.csv"));
	assert(fs::exists(output_path / "debug/start_forces.csv"));
	assert(fs::exists(output_path / "debug/wars.csv"));
	const auto countries_output = ck3eu5::common::readTextFile(output_path / "main_menu/setup/start/10_countries.txt");
	const auto diplomacy_output = ck3eu5::common::readTextFile(output_path / "main_menu/setup/start/12_diplomacy.txt");
	const auto characters_output = ck3eu5::common::readTextFile(output_path / "main_menu/setup/start/05_characters.txt");
	const auto buildings_output = ck3eu5::common::readTextFile(output_path / "main_menu/setup/start/07_cities_and_buildings.txt");
	const auto roads_output = ck3eu5::common::readTextFile(output_path / "main_menu/setup/start/09_roads.txt");
	const auto wars_output = ck3eu5::common::readTextFile(output_path / "main_menu/setup/start/16_wars.txt");
	const auto generated_country_defs_output =
		 ck3eu5::common::readTextFile(output_path / "in_game/setup/countries/00_ck3_generated_countries.txt");
	const auto localization_output =
		 ck3eu5::common::readTextFile(output_path / "in_game/localization/english/ck3_to_eu5_l_english.yml");
	const auto startup_on_action_output =
		 ck3eu5::common::readTextFile(output_path / "in_game/common/on_action/zz_ck3eu5_startup.txt");
	const auto startup_events_output =
		 ck3eu5::common::readTextFile(output_path / "in_game/events/zz_ck3eu5_startup.txt");
	assert(countries_output.find("include = \"catholic_monarchy\"") == std::string::npos);
	assert(countries_output.find("\t\t\tgovernment = {") != std::string::npos);
	assert(countries_output.find("\t\t\t\ttype = monarchy") != std::string::npos);
	assert(countries_output.find("laws = {") != std::string::npos);
	assert(countries_output.find("reforms = {") != std::string::npos);
	assert(countries_output.find("privilege = {") != std::string::npos);
	assert(countries_output.find("government_power =") != std::string::npos);
	assert(countries_output.find("\t\t\t\truler = ") != std::string::npos);
	assert(countries_output.find("ruler_term = { character = ") != std::string::npos);
	assert(countries_output.find("\t\t\tculture = ") == std::string::npos);
	assert(countries_output.find("\t\t\treligion = ") == std::string::npos);
	assert(diplomacy_output.find("subject_military_stance =") == std::string::npos);
	assert(buildings_output.find("building_manager = {") != std::string::npos);
	assert(characters_output.find("birth = london") != std::string::npos);
	assert(wars_output.find("war_manager = {") != std::string::npos);
	assert(wars_output.find("civil_war = {") != std::string::npos);
	assert(generated_country_defs_output.find("ENG = {") == std::string::npos);
	assert(generated_country_defs_output.find("NRM = {") == std::string::npos);
	assert(localization_output.find(" ENG:0 ") == std::string::npos);
	assert(localization_output.find(" NRM:0 ") == std::string::npos);
	assert(localization_output.find(" london:0 ") == std::string::npos);
	assert(roads_output.find("london = oxford") == std::string::npos);
	assert(startup_on_action_output.starts_with("\xEF\xBB\xBF"));
	assert(startup_events_output.starts_with("\xEF\xBB\xBF"));
	assert(startup_on_action_output.find("ck3eu5_on_game_start = {") != std::string::npos);
	assert(startup_on_action_output.find("on_actions = { ck3eu5_on_game_start }") != std::string::npos);
	assert(startup_on_action_output.find("trigger_event_silently = ck3eu5_startup.1") != std::string::npos);
	assert(startup_on_action_output.find("CK3EU5_VALIDATE_ON_GAME_START") != std::string::npos);
	assert(startup_on_action_output.find("CK3EU5_VALIDATE_WAR_PRESENT") != std::string::npos);
	assert(startup_events_output.find("title = ck3eu5_startup.1.t") != std::string::npos);
	assert(startup_events_output.find("desc = ck3eu5_startup.1.desc") != std::string::npos);
	assert(startup_events_output.find("orphan = yes") == std::string::npos);
	assert(startup_events_output.find("create_sub_unit_with_owner = {") != std::string::npos);
	assert(startup_events_output.find("origin = scope:location") != std::string::npos);
	assert(startup_events_output.find("CK3EU5_VALIDATE_COUNTRY_ENG_START") != std::string::npos);
	assert(startup_events_output.find("CK3EU5_VALIDATE_ARMY_PRESENT_ENG") != std::string::npos);
	assert(startup_events_output.find("CK3EU5_VALIDATE_WAR_LINK_ENG_NRM") != std::string::npos);
	assert(startup_events_output.find("CK3EU5_VALIDATE_WAR_LINK_NRM_ENG") != std::string::npos);

	auto minimal_configuration = configuration;
	const auto minimal_output_path = output_path / "minimal_government";
	minimal_configuration.output_mod_path = minimal_output_path;
	minimal_configuration.minimal_government_setup = true;
	outputter.write(eu5_world, framework, minimal_configuration, diagnostics);
	const auto minimal_countries_output =
		 ck3eu5::common::readTextFile(minimal_output_path / "main_menu/setup/start/10_countries.txt");
	assert(minimal_countries_output.find("\t\t\tgovernment = {") != std::string::npos);
	assert(minimal_countries_output.find("\t\t\t\ttype = monarchy") != std::string::npos);
	assert(minimal_countries_output.find("\t\t\t\their_selection = cognatic_primogeniture") != std::string::npos);
	assert(minimal_countries_output.find("parliament = { parliament_type = estate_parliament }") != std::string::npos);
	assert(minimal_countries_output.find("laws = {") != std::string::npos);
	assert(minimal_countries_output.find("marriage_law = monogamous_marriage") != std::string::npos);
	assert(minimal_countries_output.find("reforms = {") == std::string::npos);
	assert(minimal_countries_output.find("privilege = {") == std::string::npos);
	assert(minimal_countries_output.find("\t\t\t\tconsort = ") == std::string::npos);
	assert(minimal_countries_output.find("\t\t\t\their = ") == std::string::npos);
	assert(minimal_countries_output.find("ruler_term = { character = ") != std::string::npos);

	auto validation_configuration = minimal_configuration;
	const auto validation_output_path = output_path / "validation_monarchy";
	validation_configuration.output_mod_path = validation_output_path;
	validation_configuration.validation_force_monarchy = true;
	outputter.write(eu5_world, framework, validation_configuration, diagnostics);
	const auto validation_countries_output =
		 ck3eu5::common::readTextFile(validation_output_path / "main_menu/setup/start/10_countries.txt");
	assert(validation_countries_output.find("\t\t\tgovernment = {") != std::string::npos);
	assert(validation_countries_output.find("\t\t\t\ttype = monarchy") != std::string::npos);
	assert(validation_countries_output.find("\t\t\tinclude = \"catholic_monarchy\"") == std::string::npos);
	assert(validation_countries_output.find("\t\t\t\their_selection = cognatic_primogeniture") != std::string::npos);
	assert(validation_countries_output.find("parliament = { parliament_type = estate_parliament }") != std::string::npos);
	assert(validation_countries_output.find("laws = {") != std::string::npos);
	assert(validation_countries_output.find("marriage_law = monogamous_marriage") != std::string::npos);
	assert(validation_countries_output.find("privilege = {") == std::string::npos);
	assert(validation_countries_output.find("auxilium_et_consilium") == std::string::npos);
	assert(validation_countries_output.find("ruler_term = { character = ") != std::string::npos);

	fs::remove_all(output_path, ec);
	fs::remove_all(minimal_output_path, ec);
	fs::remove_all(validation_output_path, ec);
	std::cout << "Smoke test passed.\n";
	return 0;
}
