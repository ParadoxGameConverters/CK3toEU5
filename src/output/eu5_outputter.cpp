#include "output/eu5_outputter.h"

#include "common/filesystem_utils.h"
#include "common/logger.h"
#include "common/string_utils.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>

namespace ck3eu5::output {
namespace fs = std::filesystem;

namespace {

std::string yesNo(const bool value)
{
	return value ? "yes" : "no";
}

bool isMuslimReligion(const std::string& religion)
{
	const auto normalized = ck3eu5::common::toLower(religion);
	return normalized.find("sunni") != std::string::npos || normalized.find("shia") != std::string::npos ||
			 normalized.find("ibadi") != std::string::npos || normalized.find("muslim") != std::string::npos ||
			 normalized.find("islam") != std::string::npos;
}

std::string defaultParliamentType(const eu5::Country& country)
{
	if (!country.parliament_type.empty())
	{
		return country.parliament_type;
	}
	if (country.government_type == "monarchy" && isMuslimReligion(country.primary_religion))
	{
		return "council";
	}
	return (country.government_type == "tribe" || country.government_type == "steppe_horde") ? "assembly" : "estate_parliament";
}

std::string effectiveGovernmentType(const eu5::Country& country, const config::Configuration& configuration)
{
	if (configuration.validation_force_monarchy)
	{
		return "monarchy";
	}
	return country.government_type;
}

std::vector<std::pair<std::string, std::string>> minimalGovernmentLawsForCountry(
	 const eu5::Country& country, const std::string& government_type)
{
	if (government_type == "monarchy")
	{
		if (isMuslimReligion(country.primary_religion))
		{
			return {{"feudal_de_jure_law", "by_tradition"},
					{"medieval_levy_law", "peasant_levies"},
					{"royal_court_customs_law", "aristocratic_court_policy"},
					{"iqta_law", "efficient_tax_farming"},
					{"censorship", "limited_censorship"},
					{"education_masses_law", "basic_religious_education"},
					{"administrative_system", "feudal_administration"},
					{"cultural_traditions_law", "martial_society"},
					{"marriage_law", "muslim_marriage"},
					{"heir_religion_law", "heir_same_religion"},
					{"legal_code_law", "sharia_law_policy"},
					{"maritime_law", "protect_trade_routes"},
					{"piracy_law", "anti_piracy_policy"},
					{"distribution_of_power_law", "dop_traditional_distribution_of_power"},
					{"immigration_law", "open_borders_law"},
					{"mining_law", "nobles_mining_law"},
					{"coin_laws", "gold_and_silver_coins"}};
		}
		return {{"feudal_de_jure_law", "by_tradition"},
				{"medieval_levy_law", "peasant_levies"},
				{"royal_court_customs_law", "aristocratic_court_policy"},
				{"censorship", "limited_censorship"},
				{"education_masses_law", "basic_religious_education"},
				{"administrative_system", "feudal_administration"},
				{"cultural_traditions_law", "martial_society"},
				{"marriage_law", "monogamous_marriage"},
				{"heir_religion_law", "heir_same_religion"},
				{"legal_code_law", "civil_law_policy"},
				{"maritime_law", "protect_trade_routes"},
				{"piracy_law", "anti_piracy_policy"},
				{"distribution_of_power_law", "dop_traditional_distribution_of_power"},
				{"immigration_law", "open_borders_law"},
				{"mining_law", "nobles_mining_law"},
				{"coin_laws", "gold_and_silver_coins"}};
	}
	return {};
}

std::string formatDouble(const double value, const int precision = 3)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(precision) << value;
	std::string text = out.str();
	while (text.size() > 1 && text.ends_with('0'))
	{
		text.pop_back();
	}
	if (!text.empty() && text.ends_with('.'))
	{
		text.push_back('0');
	}
	return text;
}

std::string jsonEscape(std::string_view value)
{
	std::string result;
	result.reserve(value.size());
	for (const char c: value)
	{
		switch (c)
		{
			case '\\':
				result += "\\\\";
				break;
			case '"':
				result += "\\\"";
				break;
			case '\n':
				result += "\\n";
				break;
			case '\r':
				result += "\\r";
				break;
			case '\t':
				result += "\\t";
				break;
			default:
				result.push_back(c);
				break;
		}
	}
	return result;
}

std::string csvEscape(std::string_view value)
{
	std::string result = "\"";
	for (const char c: value)
	{
		if (c == '"')
		{
			result += "\"\"";
		}
		else
		{
			result.push_back(c);
		}
	}
	result.push_back('"');
	return result;
}

uint32_t fnv1a(std::string_view value)
{
	uint32_t hash = 2166136261u;
	for (const unsigned char c: value)
	{
		hash ^= c;
		hash *= 16777619u;
	}
	return hash;
}

std::string rgbString(const int r, const int g, const int b)
{
	std::ostringstream out;
	out << "rgb { " << r << ' ' << g << ' ' << b << " }";
	return out.str();
}

std::string renderBuildings(const eu5::World& world);

bool isVanillaCountryTag(const eu5::WorldFramework& framework, const std::string& tag)
{
	return framework.colors.contains(tag);
}

bool isVanillaCountryLocalizationKey(const eu5::WorldFramework& framework, const std::string& key)
{
	if (isVanillaCountryTag(framework, key))
	{
		return true;
	}

	static constexpr std::string_view adjective_suffix = "_ADJ";
	if (key.size() > adjective_suffix.size() &&
		key.compare(key.size() - adjective_suffix.size(), adjective_suffix.size(), adjective_suffix) == 0)
	{
		return isVanillaCountryTag(framework, key.substr(0, key.size() - adjective_suffix.size()));
	}

	return false;
}

bool isVanillaLocationLocalizationKey(const eu5::WorldFramework& framework, const std::string& key)
{
	return framework.locations.contains(key);
}

eu5::CountryColorDefinition makeFallbackColorDefinition(const std::string& tag)
{
	const auto hash = fnv1a(tag);
	auto pick = [hash](const int shift, const int floor, const int range) {
		return floor + static_cast<int>((hash >> shift) % static_cast<uint32_t>(range));
	};

	const int r = pick(0, 55, 145);
	const int g = pick(8, 55, 145);
	const int b = pick(16, 55, 145);

	eu5::CountryColorDefinition definition;
	definition.tag = tag;
	definition.color = rgbString(r, g, b);
	definition.color2 = rgbString(std::min(255, r + 35), std::min(255, g + 35), std::min(255, b + 35));
	definition.color3 = rgbString(std::max(0, r - 35), std::max(0, g - 35), std::max(0, b - 35));
	definition.unit_color0 = definition.color;
	definition.unit_color1 = definition.color2;
	definition.unit_color2 = definition.color3;
	definition.description_category = "administrative";
	definition.difficulty = 3;
	return definition;
}

const eu5::CountryColorDefinition& colorDefinitionForTag(const eu5::WorldFramework& framework,
	 const std::string& tag,
	 std::map<std::string, eu5::CountryColorDefinition>& fallback_cache)
{
	if (const auto it = framework.colors.find(tag); it != framework.colors.end())
	{
		return it->second;
	}
	auto [it, inserted] = fallback_cache.emplace(tag, makeFallbackColorDefinition(tag));
	return it->second;
}

std::string renderMetadataJson(const config::Configuration& configuration)
{
	std::ostringstream out;
	out << "{\n";
	out << "  \"name\": \"" << jsonEscape(configuration.mod_name) << "\",\n";
	out << "  \"id\": \"" << jsonEscape(configuration.mod_id) << "\",\n";
	out << "  \"version\": \"" << jsonEscape(configuration.mod_version) << "\",\n";
	out << "  \"supported_game_version\": \"" << jsonEscape(configuration.supported_game_version) << "\",\n";
	out << "  \"short_description\": \"Generated by the CK3 to EU5 converter prototype.\",\n";
	out << "  \"tags\": [\"Converter\", \"Total Conversion\"],\n";
	out << "  \"relationships\": [],\n";
	out << "  \"game_custom_data\": {\n";
	out << "    \"replace_paths\": []\n";
	out << "  }\n";
	out << "}\n";
	return out.str();
}

std::string renderInstitutionManager(const eu5::World& world)
{
	std::string birth_place;
	for (const auto& [tag, country]: world.countries)
	{
		if (!country.capital_location.empty())
		{
			birth_place = country.capital_location;
			break;
		}
	}
	if (birth_place.empty() && !world.locations.empty())
	{
		birth_place = world.locations.begin()->first;
	}

	std::ostringstream out;
	out << "# Generated by CK3ToEU5 prototype\n";
	out << "institution_manager = {\n";
	out << "\tinstitutions = {\n";
	out << "\t\tfeudalism = {\n";
	out << "\t\t\tactive = yes\n";
	if (!birth_place.empty())
	{
		out << "\t\t\tbirth_place = " << birth_place << '\n';
	}
	out << "\t\t}\n";
	out << "\t}\n";
	out << "}\n";
	return out.str();
}

std::string renderDynasties(const eu5::World& world)
{
	std::ostringstream out;
	out << "# Dynasties must be created before characters.\n";
	out << "dynasty_manager = {\n";
	for (const auto& [key, dynasty]: world.dynasties)
	{
		out << "\t" << key << " = {\n";
		out << "\t\tname = { name = " << dynasty.loc_name_key << " }\n";
		out << "\t\tdynasty_name_type = location\n";
		if (!dynasty.home_location.empty())
		{
			out << "\t\thome = " << dynasty.home_location << '\n';
		}
		out << "\t}\n";
	}
	out << "}\n";
	return out.str();
}

std::string renderCharacters(const eu5::World& world)
{
	std::ostringstream out;
	out << "character_db = {\n";
	for (const auto& [key, character]: world.characters)
	{
		out << "\t" << key << " = {\n";
		out << "\t\tfirst_name = { name = " << character.first_name_loc_key << " }\n";
		out << "\t\tlast_name = { name = " << character.last_name_loc_key << " }\n";
		out << "\t\tculture = " << character.culture << '\n';
		out << "\t\treligion = " << character.religion << '\n';
		out << "\t\tadm = " << character.adm << '\n';
		out << "\t\tdip = " << character.dip << '\n';
		out << "\t\tmil = " << character.mil << '\n';
		out << "\t\tbirth_date = " << character.birth_date << '\n';
		if (!character.birth_location.empty())
		{
			out << "\t\tbirth = " << character.birth_location << '\n';
		}
		if (!character.death_date.empty())
		{
			out << "\t\tdeath_date = " << character.death_date << '\n';
		}
		out << "\t\tfemale = " << yesNo(character.female) << '\n';
		out << "\t\ttag = " << character.tag << '\n';
		if (!character.dynasty_key.empty())
		{
			out << "\t\tdynasty = " << character.dynasty_key << '\n';
		}
		out << "\t}\n";
	}
	out << "}\n";
	return out.str();
}

std::string renderPops(const eu5::World& world)
{
	std::ostringstream out;
	out << "locations = {\n";
	for (const auto& [key, location]: world.locations)
	{
		out << "\t" << key << " = {\n";
		for (const auto& pop: location.pops)
		{
			out << "\t\tdefine_pop = {\n";
			out << "\t\t\tculture = " << pop.culture << '\n';
			out << "\t\t\treligion = " << pop.religion << '\n';
			out << "\t\t\ttype = " << pop.type << '\n';
			out << "\t\t\tsize = " << formatDouble(pop.size) << '\n';
			out << "\t\t}\n";
		}
		out << "\t}\n";
	}
	out << "}\n";
	return out.str();
}

std::string renderCitiesAndBuildings(const eu5::World& world)
{
	std::ostringstream out;
	out << "locations = {\n";
	for (const auto& [key, location]: world.locations)
	{
		out << "\t" << key << " = {\n";
		if (!location.rank.empty() && location.rank != "rural_settlement")
		{
			out << "\t\trank = " << location.rank << '\n';
		}
		if (!location.town_setup.empty())
		{
			out << "\t\ttown_setup = " << location.town_setup << '\n';
		}
		out << "\t}\n";
	}
	out << "}\n\n";
	out << renderBuildings(world);
	return out.str();
}

std::string renderCountries(const eu5::World& world, const config::Configuration& configuration)
{
	std::ostringstream out;
	out << "countries = {\n";
	out << "\tcountries = {\n";
	for (const auto& [tag, country]: world.countries)
	{
		const auto government_type = effectiveGovernmentType(country, configuration);
		out << "\t\t" << tag << " = {\n";
		if (!country.owned_core_locations.empty())
		{
			out << "\t\t\town_control_core = {";
			for (const auto& location_key: country.owned_core_locations)
			{
				out << ' ' << location_key;
			}
			out << " }\n";
		}
		if (!country.capital_location.empty())
		{
			out << "\t\t\tcapital = " << country.capital_location << '\n';
		}
		if (!country.ruler_character_key.empty())
		{
			if (const auto character_it = world.characters.find(country.ruler_character_key);
				 character_it != world.characters.end() && !character_it->second.dynasty_key.empty())
			{
				out << "\t\t\tdynasty = " << character_it->second.dynasty_key << '\n';
			}
		}
		out << "\t\t\tcountry_rank = " << country.country_rank << '\n';
		out << "\t\t\tstarting_technology_level = " << country.starting_technology_level << '\n';
		if (!country.accepted_cultures.empty())
		{
			out << "\t\t\taccepted_cultures = {";
			for (const auto& culture: country.accepted_cultures)
			{
				out << ' ' << culture;
			}
			out << " }\n";
		}
		if (!country.tolerated_cultures.empty())
		{
			out << "\t\t\ttolerated_cultures = {";
			for (const auto& culture: country.tolerated_cultures)
			{
				out << ' ' << culture;
			}
			out << " }\n";
		}
		out << "\t\t\tcurrency_data = {\n";
		out << "\t\t\t\tgold = " << country.gold << '\n';
		out << "\t\t\t\tprestige = " << country.prestige << '\n';
		out << "\t\t\t\tgovernment_power = " << country.government_power << '\n';
		out << "\t\t\t}\n";
		out << "\t\t\tgovernment = {\n";
		out << "\t\t\t\ttype = " << government_type << '\n';
		if (!country.heir_selection.empty())
		{
			out << "\t\t\t\their_selection = " << country.heir_selection << '\n';
		}
		if (!country.ruler_character_key.empty())
		{
			out << "\t\t\t\truler = " << country.ruler_character_key << '\n';
			out << "\t\t\t\truler_term = { character = " << country.ruler_character_key << " start_date = " << world.date << " }\n";
		}
		if (configuration.minimal_government_setup)
		{
			out << "\t\t\t\tcentralization_vs_decentralization = " << country.centralization_vs_decentralization << '\n';
			out << "\t\t\t\ttraditionalist_vs_innovative = " << country.traditionalist_vs_innovative << '\n';
			out << "\t\t\t\tspiritualist_vs_humanist = " << country.spiritualist_vs_humanist << '\n';
			out << "\t\t\t\taristocracy_vs_plutocracy = " << country.aristocracy_vs_plutocracy << '\n';
			out << "\t\t\t\tserfdom_vs_free_subjects = " << country.serfdom_vs_free_subjects << '\n';
			out << "\t\t\t\tmercantilism_vs_free_trade = " << country.mercantilism_vs_free_trade << '\n';
			out << "\t\t\t\tbelligerent_vs_conciliatory = " << country.belligerent_vs_conciliatory << '\n';
			out << "\t\t\t\tquality_vs_quantity = " << country.quality_vs_quantity << '\n';
			out << "\t\t\t\toffensive_vs_defensive = " << country.offensive_vs_defensive << '\n';
			out << "\t\t\t\tland_vs_naval = " << country.land_vs_naval << '\n';
			out << "\t\t\t\tcapital_economy_vs_traditional_economy = " << country.capital_economy_vs_traditional_economy << '\n';
			out << "\t\t\t\tindividualism_vs_communalism = " << country.individualism_vs_communalism << '\n';
			out << "\t\t\t\toutward_vs_inward = " << country.outward_vs_inward << '\n';
			eu5::Country effective_country = country;
			effective_country.government_type = government_type;
			out << "\t\t\t\tparliament = { parliament_type = " << defaultParliamentType(effective_country) << " }\n";
			const auto baseline_laws = minimalGovernmentLawsForCountry(effective_country, government_type);
			if (!baseline_laws.empty())
			{
				out << "\t\t\t\tlaws = {\n";
				for (const auto& [law_name, law_value]: baseline_laws)
				{
					out << "\t\t\t\t\t" << law_name << " = " << law_value << '\n';
				}
				out << "\t\t\t\t}\n";
			}
		}
		else
		{
			if (!country.consort_character_key.empty() &&
				 (government_type == "monarchy" || government_type == "tribe" || government_type == "steppe_horde"))
			{
				out << "\t\t\t\tconsort = " << country.consort_character_key << '\n';
			}
			if (!country.heir_character_key.empty() &&
				 (government_type == "monarchy" || government_type == "tribe" || government_type == "steppe_horde"))
			{
				out << "\t\t\t\their = " << country.heir_character_key << '\n';
			}
			out << "\t\t\t\tcentralization_vs_decentralization = " << country.centralization_vs_decentralization << '\n';
			out << "\t\t\t\ttraditionalist_vs_innovative = " << country.traditionalist_vs_innovative << '\n';
			out << "\t\t\t\tspiritualist_vs_humanist = " << country.spiritualist_vs_humanist << '\n';
			out << "\t\t\t\taristocracy_vs_plutocracy = " << country.aristocracy_vs_plutocracy << '\n';
			out << "\t\t\t\tserfdom_vs_free_subjects = " << country.serfdom_vs_free_subjects << '\n';
			out << "\t\t\t\tmercantilism_vs_free_trade = " << country.mercantilism_vs_free_trade << '\n';
			out << "\t\t\t\tbelligerent_vs_conciliatory = " << country.belligerent_vs_conciliatory << '\n';
			out << "\t\t\t\tquality_vs_quantity = " << country.quality_vs_quantity << '\n';
			out << "\t\t\t\toffensive_vs_defensive = " << country.offensive_vs_defensive << '\n';
			out << "\t\t\t\tland_vs_naval = " << country.land_vs_naval << '\n';
			out << "\t\t\t\tcapital_economy_vs_traditional_economy = " << country.capital_economy_vs_traditional_economy << '\n';
			out << "\t\t\t\tindividualism_vs_communalism = " << country.individualism_vs_communalism << '\n';
			out << "\t\t\t\toutward_vs_inward = " << country.outward_vs_inward << '\n';
			eu5::Country effective_country = country;
			effective_country.government_type = government_type;
			out << "\t\t\t\tparliament = { parliament_type = " << defaultParliamentType(effective_country) << " }\n";
			if (!country.reforms.empty())
			{
				out << "\t\t\t\treforms = {";
				for (const auto& reform: country.reforms)
				{
					out << ' ' << reform;
				}
				out << " }\n";
			}
			if (!country.privileges.empty())
			{
				out << "\t\t\t\tprivilege = {";
				for (const auto& privilege: country.privileges)
				{
					out << ' ' << privilege;
				}
				out << " }\n";
			}
			if (!country.laws.empty())
			{
				out << "\t\t\t\tlaws = {\n";
				for (const auto& [law_name, law_value]: country.laws)
				{
					out << "\t\t\t\t\t" << law_name << " = " << law_value << '\n';
				}
				out << "\t\t\t\t}\n";
			}
		}
		out << "\t\t\t}\n";
		out << "\t\t}\n";
	}
	out << "\t}\n";
	out << "}\n";
	return out.str();
}

std::string renderDiplomacy(const eu5::World& world)
{
	std::ostringstream out;
	out << "diplomacy_manager = {\n";
	for (const auto& relation: world.subject_relations)
	{
		out << "\tdependency = {\n";
		out << "\t\tfirst = " << relation.liege_tag << '\n';
		out << "\t\tsecond = " << relation.subject_tag << '\n';
		out << "\t\tsubject_type = " << relation.subject_type << '\n';
		out << "\t\tstart_date = " << relation.start_date << '\n';
		out << "\t}\n";
	}
	for (const auto& relation: world.scripted_relations)
	{
		out << '\t' << (relation.mutual ? "scripted_mutual" : "scripted_oneway") << " = {\n";
		out << "\t\tfirst = " << relation.first_tag << '\n';
		out << "\t\tsecond = " << relation.second_tag << '\n';
		out << "\t\ttype = " << relation.type << '\n';
		out << "\t}\n";
	}
	out << "}\n";
	return out.str();
}

std::string renderOpinions(const eu5::World& world)
{
	std::ostringstream out;
	out << "diplomacy_manager = {\n";
	for (const auto& opinion: world.opinions)
	{
		out << "\topinion = { first = " << opinion.first_tag << " second = " << opinion.second_tag << " type = " << opinion.type
			 << " }\n";
	}
	out << "}\n";
	return out.str();
}

std::string renderRivals(const eu5::World& world)
{
	std::ostringstream out;
	out << "diplomacy_manager = {\n";
	for (const auto& rival: world.rivals)
	{
		out << "\trival = { first = " << rival.first_tag << " second = " << rival.second_tag << " }\n";
	}
	out << "}\n";
	return out.str();
}

std::string renderMarkets(const eu5::World& world)
{
	std::ostringstream out;
	out << "market_manager = {\n";
	for (const auto& market: world.markets)
	{
		if (!market.location.empty())
		{
			out << "\tadd_market = " << market.location << '\n';
		}
	}
	out << "}\n";
	return out.str();
}

std::string renderBuildings(const eu5::World& world)
{
	std::ostringstream out;
	out << "building_manager = {\n";
	for (const auto& building: world.buildings)
	{
		out << '\t' << building.type << " = {\n";
		out << "\t\tlocation = " << building.location << '\n';
		if (!building.tag.empty())
		{
			out << "\t\ttag = " << building.tag << '\n';
		}
		out << "\t\tlevel = " << building.level << '\n';
		out << "\t}\n";
	}
	out << "}\n";
	return out.str();
}

std::string renderWars(const eu5::World& world)
{
	std::ostringstream out;
	out << "war_manager = {\n";
	for (const auto& war: world.wars)
	{
		out << '\t' << (war.civil_war ? "civil_war" : "war") << " = {\n";
		out << "\t\twar_name = {\n";
		out << "\t\t\tname = " << '"' << war.name << '"' << '\n';
		out << "\t\t\tordinal = " << war.ordinal << '\n';
		if (!war.first_tag.empty())
		{
			out << "\t\t\tfirst = {\n";
			out << "\t\t\t\tname = " << '"' << war.first_tag << '"' << '\n';
			out << "\t\t\t}\n";
		}
		if (!war.second_tag.empty())
		{
			out << "\t\t\tsecond = {\n";
			out << "\t\t\t\tname = " << '"' << war.second_tag << '"' << '\n';
			out << "\t\t\t}\n";
		}
		out << "\t\t}\n";
		out << "\t\tstart_date = " << war.start_date << '\n';
		out << "\t\taction = " << war.action_date << '\n';
		for (const auto& participant: war.attackers)
		{
			out << "\t\tattacker = {\n";
			out << "\t\t\tcountry = " << participant.tag << '\n';
			out << "\t\t\trequest = {\n";
			if (!participant.caller_tag.empty())
			{
				out << "\t\t\t\tcaller = " << participant.caller_tag << '\n';
			}
			out << "\t\t\t\treason = " << participant.reason << '\n';
			if (!participant.which.empty() && participant.reason == "Scripted")
			{
				out << "\t\t\t\twhich = " << participant.which << '\n';
			}
			out << "\t\t\t}\n";
			out << "\t\t}\n";
		}
		for (const auto& participant: war.defenders)
		{
			out << "\t\tdefender = {\n";
			out << "\t\t\tcountry = " << participant.tag << '\n';
			out << "\t\t\trequest = {\n";
			if (!participant.caller_tag.empty())
			{
				out << "\t\t\t\tcaller = " << participant.caller_tag << '\n';
			}
			out << "\t\t\t\treason = " << participant.reason << '\n';
			if (!participant.which.empty() && participant.reason == "Scripted")
			{
				out << "\t\t\t\twhich = " << participant.which << '\n';
			}
			out << "\t\t\t}\n";
			out << "\t\t}\n";
		}
		out << "\t}\n";
	}
	out << "}\n";
	return out.str();
}

std::string renderRoads(const eu5::World& world)
{
	std::ostringstream out;
	out << "road_network = {\n";
	for (const auto& road: world.roads)
	{
		out << '\t' << road.from << " = " << road.to << '\n';
	}
	out << "}\n";
	return out.str();
}

std::string renderDevelopment(const eu5::World& world)
{
	std::ostringstream out;
	out << "development = {\n";
	for (const auto& [key, location]: world.locations)
	{
		out << '\t' << key << " = " << formatDouble(location.development) << '\n';
	}
	out << "}\n";
	return out.str();
}

std::string renderReligionSetup()
{
	std::ostringstream out;
	out << "building_manager = {\n";
	out << "}\n\n";
	out << "religion_manager = {\n";
	out << "}\n";
	return out.str();
}

std::string renderStartupOnAction(const eu5::World& world)
{
	std::set<std::string> tags;
	for (const auto& force: world.start_forces)
	{
		tags.insert(force.tag);
	}

	std::ostringstream out;
	out << "ck3eu5_on_game_start = {\n";
	out << "\teffect = {\n";
	if (!world.wars.empty())
	{
		out << "\t\tif = {\n";
		out << "\t\t\tlimit = { debug_only = yes }\n";
		out << "\t\t\tdebug_log = \"CK3EU5_VALIDATE_ON_GAME_START\"\n";
		out << "\t\t\tdebug_log_date = yes\n";
		out << "\t\t\ttest_log = { name = ck3eu5_validation text = \"CK3EU5_VALIDATE_ON_GAME_START\" }\n";
		out << "\t\t\tevery_war = {\n";
		out << "\t\t\t\tdebug_log = \"CK3EU5_VALIDATE_WAR_PRESENT\"\n";
		out << "\t\t\t\tdebug_log_scopes = yes\n";
		out << "\t\t\t\ttest_log = { name = ck3eu5_validation_war text = \"CK3EU5_VALIDATE_WAR_PRESENT\" }\n";
		out << "\t\t\t\tevery_war_participant = {\n";
		out << "\t\t\t\t\tdebug_log = \"CK3EU5_VALIDATE_WAR_PARTICIPANT\"\n";
		out << "\t\t\t\t\tdebug_log_scopes = yes\n";
		out << "\t\t\t\t\ttest_log = { name = ck3eu5_validation_war_participant text = "
			   "\"CK3EU5_VALIDATE_WAR_PARTICIPANT\" }\n";
		out << "\t\t\t\t}\n";
		out << "\t\t\t}\n";
		out << "\t\t}\n";
	}
	for (const auto& tag: tags)
	{
		out << "\t\tc:" << tag << " = { trigger_event_silently = ck3eu5_startup.1 }\n";
	}
	out << "\t}\n";
	out << "}\n\n";
	out << "on_game_start = {\n";
	out << "\ton_actions = { ck3eu5_on_game_start }\n";
	out << "}\n";
	return out.str();
}

std::string renderStartupEvents(const eu5::World& world)
{
	std::map<std::string, std::vector<const eu5::StartForce*>> forces_by_tag;
	for (const auto& force: world.start_forces)
	{
		forces_by_tag[force.tag].push_back(&force);
	}
	std::map<std::string, std::set<std::string>> wartime_opponents_by_tag;
	std::map<std::string, std::set<std::string>> wartime_declare_targets_by_tag;
	for (const auto& war: world.wars)
	{
		for (const auto& attacker: war.attackers)
		{
			for (const auto& defender: war.defenders)
			{
				if (!attacker.tag.empty() && !defender.tag.empty() && attacker.tag != defender.tag)
				{
					wartime_opponents_by_tag[attacker.tag].insert(defender.tag);
					wartime_opponents_by_tag[defender.tag].insert(attacker.tag);
					wartime_declare_targets_by_tag[attacker.tag].insert(defender.tag);
				}
			}
		}
	}

	std::ostringstream out;
	out << "namespace = ck3eu5_startup\n\n";
	out << "ck3eu5_startup.1 = {\n";
	out << "\ttype = country_event\n";
	out << "\ttitle = ck3eu5_startup.1.t\n";
	out << "\tdesc = ck3eu5_startup.1.desc\n";
	out << "\thide_portraits = yes\n";
	out << "\timmediate = {\n";
	for (const auto& [tag, forces]: forces_by_tag)
	{
		const bool has_army = std::any_of(forces.begin(), forces.end(), [](const auto* force) { return force->branch == "army"; });
		const bool has_navy = std::any_of(forces.begin(), forces.end(), [](const auto* force) { return force->branch == "navy"; });

		out << "\t\tif = {\n";
		out << "\t\t\tlimit = { tag = " << tag << " }\n";
		out << "\t\t\tif = {\n";
		out << "\t\t\t\tlimit = { debug_only = yes }\n";
		out << "\t\t\t\tdebug_log = \"CK3EU5_VALIDATE_COUNTRY_" << tag << "_START\"\n";
		out << "\t\t\t\tdebug_log_scopes = yes\n";
		out << "\t\t\t\ttest_log = { name = ck3eu5_validation_country text = \"CK3EU5_VALIDATE_COUNTRY_" << tag
			   << "_START\" }\n";
		out << "\t\t\t}\n";
		for (const auto* force: forces)
		{
			out << "\t\t\t# " << force->branch << ' ' << force->key << '\n';
			out << "\t\t\tlocation:" << force->location << " = {\n";
			for (const auto& unit: force->units)
			{
				out << "\t\t\t\twhile = {\n";
				out << "\t\t\t\t\tcount = " << unit.count << '\n';
				out << "\t\t\t\t\tcreate_sub_unit_with_owner = {\n";
				out << "\t\t\t\t\t\ttype = " << unit.type << '\n';
				out << "\t\t\t\t\t\towner = root\n";
				out << "\t\t\t\t\t\torigin = scope:location\n";
				out << "\t\t\t\t\t}\n";
				out << "\t\t\t\t}\n";
			}
			out << "\t\t\t}\n";
		}
		out << "\t\t\tforce_recalc_country_active_status = root\n";
		out << "\t\t\tif = {\n";
		out << "\t\t\t\tlimit = { debug_only = yes }\n";
		if (has_army)
		{
			out << "\t\t\t\tif = {\n";
			out << "\t\t\t\t\tlimit = { any_army = { } }\n";
			out << "\t\t\t\t\tdebug_log = \"CK3EU5_VALIDATE_ARMY_PRESENT_" << tag << "\"\n";
			out << "\t\t\t\t\ttest_log = { name = ck3eu5_validation_army text = \"CK3EU5_VALIDATE_ARMY_PRESENT_" << tag
				   << "\" }\n";
			out << "\t\t\t\t}\n";
			out << "\t\t\t\telse = {\n";
			out << "\t\t\t\t\terror_log = \"CK3EU5_VALIDATE_ARMY_MISSING_" << tag << "\"\n";
			out << "\t\t\t\t}\n";
		}
		if (has_navy)
		{
			out << "\t\t\t\tif = {\n";
			out << "\t\t\t\t\tlimit = { any_navy = { } }\n";
			out << "\t\t\t\t\tdebug_log = \"CK3EU5_VALIDATE_NAVY_PRESENT_" << tag << "\"\n";
			out << "\t\t\t\t\ttest_log = { name = ck3eu5_validation_navy text = \"CK3EU5_VALIDATE_NAVY_PRESENT_" << tag
				   << "\" }\n";
			out << "\t\t\t\t}\n";
			out << "\t\t\t\telse = {\n";
			out << "\t\t\t\t\terror_log = \"CK3EU5_VALIDATE_NAVY_MISSING_" << tag << "\"\n";
			out << "\t\t\t\t}\n";
		}
		if (const auto opponents_it = wartime_opponents_by_tag.find(tag); opponents_it != wartime_opponents_by_tag.end())
		{
			for (const auto& opponent_tag: opponents_it->second)
			{
				const bool should_declare = wartime_declare_targets_by_tag.contains(tag) &&
										  wartime_declare_targets_by_tag.at(tag).contains(opponent_tag);
				if (should_declare)
				{
					out << "\t\t\t\tif = {\n";
					out << "\t\t\t\t\tlimit = { NOT = { is_at_war_with = c:" << opponent_tag << " } }\n";
					out << "\t\t\t\t\tdeclare_war = c:" << opponent_tag << '\n';
					out << "\t\t\t\t\tdebug_log = \"CK3EU5_VALIDATE_WAR_DECLARE_" << tag << "_" << opponent_tag << "\"\n";
					out << "\t\t\t\t\ttest_log = { name = ck3eu5_validation_war_declare text = "
						   "\"CK3EU5_VALIDATE_WAR_DECLARE_" << tag << "_" << opponent_tag << "\" }\n";
					out << "\t\t\t\t}\n";
				}
				out << "\t\t\t\tif = {\n";
				out << "\t\t\t\t\tlimit = { is_at_war_with = c:" << opponent_tag << " }\n";
				out << "\t\t\t\t\tdebug_log = \"CK3EU5_VALIDATE_WAR_LINK_" << tag << "_" << opponent_tag << "\"\n";
				out << "\t\t\t\t\ttest_log = { name = ck3eu5_validation_war_link text = \"CK3EU5_VALIDATE_WAR_LINK_" << tag
					   << "_" << opponent_tag << "\" }\n";
				out << "\t\t\t\t}\n";
				if (should_declare)
				{
					out << "\t\t\t\telse = {\n";
					out << "\t\t\t\t\terror_log = \"CK3EU5_VALIDATE_WAR_LINK_MISSING_" << tag << "_" << opponent_tag << "\"\n";
					out << "\t\t\t\t}\n";
				}
			}
		}
		out << "\t\t\t}\n";
		out << "\t\t}\n";
	}
	out << "\t}\n";
	out << "\toption = { name = ck3eu5_startup.1.a }\n";
	out << "}\n";
	return out.str();
}

std::string renderCountryDefinitions(const eu5::World& world,
	 const eu5::WorldFramework& framework,
	 std::map<std::string, eu5::CountryColorDefinition>& fallback_cache)
{
	std::ostringstream out;
	out << "# Generated by CK3ToEU5 prototype\n";
	for (const auto& [tag, country]: world.countries)
	{
		if (isVanillaCountryTag(framework, tag))
		{
			continue;
		}
		const auto& color = colorDefinitionForTag(framework, tag, fallback_cache);
		out << tag << " = {\n";
		out << "\tcolor = " << color.color << '\n';
		out << "\tcolor2 = " << color.color2 << '\n';
		out << "\tcolor3 = " << color.color3 << '\n';
		out << "\tunit_color0 = " << color.unit_color0 << '\n';
		out << "\tunit_color1 = " << color.unit_color1 << '\n';
		out << "\tunit_color2 = " << color.unit_color2 << '\n';
		out << "\tculture_definition = " << (country.primary_culture.empty() ? "english" : country.primary_culture) << '\n';
		out << "\treligion_definition = " << (country.primary_religion.empty() ? "catholic" : country.primary_religion) << '\n';
		out << "\tdescription_category = " << color.description_category << '\n';
		out << "\tdifficulty = " << color.difficulty << '\n';
		out << "}\n\n";
	}
	return out.str();
}

std::string renderTownSetups()
{
	std::ostringstream out;
	out << "ck3_converter_town = {\n";
	out << "\tbrewery = 1\n";
	out << "\ttemple = 1\n";
	out << "}\n\n";
	out << "ck3_converter_city = {\n";
	out << "\tbrewery = 1\n";
	out << "\ttemple = 1\n";
	out << "\ttools_guild = 1\n";
	out << "}\n\n";
	out << "ck3_converter_capital_town = {\n";
	out << "\tbrewery = 1\n";
	out << "\ttemple = 1\n";
	out << "\ttools_guild = 1\n";
	out << "}\n\n";
	out << "ck3_converter_capital_city = {\n";
	out << "\tbrewery = 2\n";
	out << "\ttemple = 1\n";
	out << "\ttools_guild = 2\n";
	out << "}\n";
	return out.str();
}

std::string renderLocalization(const eu5::World& world, const eu5::WorldFramework& framework)
{
	std::map<std::string, std::string> entries = world.localization;
	entries.emplace("ck3eu5_startup.1.t", "CK3 to EU5 startup");
	entries.emplace("ck3eu5_startup.1.desc", "Internal startup event used by the CK3 to EU5 converter.");
	entries.emplace("ck3eu5_startup.1.a", "Continue");
	for (const auto& [key, location]: world.locations)
	{
		if (!isVanillaLocationLocalizationKey(framework, key))
		{
			if (const auto* definition = framework.getLocation(key); definition && !definition->display_name.empty())
			{
				entries.emplace(key, definition->display_name);
			}
		}
	}

	std::ostringstream out;
	out << "l_english:\n";
	for (const auto& [key, value]: entries)
	{
		if (isVanillaCountryLocalizationKey(framework, key) || isVanillaLocationLocalizationKey(framework, key))
		{
			continue;
		}
		out << ' ' << key << ":0 \"" << common::makeSafeLocalization(value) << "\"\n";
	}
	return out.str();
}

std::string renderWorldSummary(const eu5::World& world)
{
	std::ostringstream out;
	out << "CK3ToEU5 generated world summary\n";
	out << "Date: " << world.date << "\n";
	out << "Countries: " << world.countries.size() << "\n";
	out << "Locations: " << world.locations.size() << "\n";
	out << "Characters: " << world.characters.size() << "\n";
	out << "Dynasties: " << world.dynasties.size() << "\n";
	out << "Subject relations: " << world.subject_relations.size() << "\n";
	out << "Scripted relations: " << world.scripted_relations.size() << "\n";
	out << "Opinions: " << world.opinions.size() << "\n";
	out << "Rivals: " << world.rivals.size() << "\n";
	out << "Markets: " << world.markets.size() << "\n";
	out << "Buildings: " << world.buildings.size() << "\n";
	out << "Force plans: " << world.force_plans.size() << "\n";
	out << "Start forces: " << world.start_forces.size() << "\n";
	out << "Wars: " << world.wars.size() << "\n";
	out << "Roads: " << world.roads.size() << "\n\n";

	for (const auto& [tag, country]: world.countries)
	{
		out << tag << " -> " << country.display_name << '\n';
		out << "  title: " << country.source_title_key << '\n';
		out << "  capital: " << country.capital_location << '\n';
		out << "  culture/religion: " << country.primary_culture << " / " << country.primary_religion << '\n';
		out << "  government: " << country.government_type << '\n';
		out << "  heir selection: " << country.heir_selection << '\n';
		out << "  locations: " << country.owned_core_locations.size() << "\n\n";
	}
	return out.str();
}

std::string renderCountriesCsv(const eu5::World& world)
{
	std::ostringstream out;
	out << "tag,display_name,source_title,capital,rank,government,heir_selection,culture,religion,technology,gold,prestige,government_power,location_count\n";
	for (const auto& [tag, country]: world.countries)
	{
		out << csvEscape(tag) << ','
			 << csvEscape(country.display_name) << ','
			 << csvEscape(country.source_title_key) << ','
			 << csvEscape(country.capital_location) << ','
			 << csvEscape(country.country_rank) << ','
			 << csvEscape(country.government_type) << ','
			 << csvEscape(country.heir_selection) << ','
			 << csvEscape(country.primary_culture) << ','
			 << csvEscape(country.primary_religion) << ','
			 << country.starting_technology_level << ',' << country.gold << ',' << country.prestige << ',' << country.government_power << ','
			 << country.owned_core_locations.size() << '\n';
	}
	return out.str();
}

std::string renderLocationsCsv(const eu5::World& world)
{
	std::ostringstream out;
	out << "location,owner,rank,development,pop_count,province_definition,region,area\n";
	for (const auto& [key, location]: world.locations)
	{
		out << csvEscape(key) << ',' << csvEscape(location.owner_tag) << ',' << csvEscape(location.rank) << ','
			 << formatDouble(location.development) << ',' << location.pops.size() << ',' << csvEscape(location.province_definition) << ','
			 << csvEscape(location.region) << ',' << csvEscape(location.area) << '\n';
	}
	return out.str();
}

std::string renderCharactersCsv(const eu5::World& world)
{
	std::ostringstream out;
	out << "character_key,tag,dynasty,culture,religion,birth_date,birth_location,death_date,female\n";
	for (const auto& [key, character]: world.characters)
	{
		out << csvEscape(key) << ',' << csvEscape(character.tag) << ',' << csvEscape(character.dynasty_key) << ','
			 << csvEscape(character.culture) << ',' << csvEscape(character.religion) << ',' << csvEscape(character.birth_date) << ','
			 << csvEscape(character.birth_location) << ',' << csvEscape(character.death_date) << ','
			 << (character.female ? "yes" : "no") << '\n';
	}
	return out.str();
}

std::string renderSubjectCsv(const eu5::World& world)
{
	std::ostringstream out;
	out << "liege,subject,subject_type,subject_military_stance,start_date\n";
	for (const auto& relation: world.subject_relations)
	{
		out << csvEscape(relation.liege_tag) << ',' << csvEscape(relation.subject_tag) << ','
			 << csvEscape(relation.subject_type) << ',' << csvEscape(relation.subject_military_stance) << ','
			 << csvEscape(relation.start_date) << '\n';
	}
	return out.str();
}

std::string renderMarketsCsv(const eu5::World& world)
{
	std::ostringstream out;
	out << "location,owner_tag,score\n";
	for (const auto& market: world.markets)
	{
		out << csvEscape(market.location) << ',' << csvEscape(market.owner_tag) << ',' << formatDouble(market.score) << '\n';
	}
	return out.str();
}

std::string renderBuildingsCsv(const eu5::World& world)
{
	std::ostringstream out;
	out << "type,location,tag,level\n";
	for (const auto& building: world.buildings)
	{
		out << csvEscape(building.type) << ',' << csvEscape(building.location) << ',' << csvEscape(building.tag) << ','
			 << building.level << '\n';
	}
	return out.str();
}

std::string renderForcePlansCsv(const eu5::World& world)
{
	std::ostringstream out;
	out << "key,tag,branch,home_location,stance,levy_estimate,standing_estimate\n";
	for (const auto& plan: world.force_plans)
	{
		out << csvEscape(plan.key) << ',' << csvEscape(plan.tag) << ',' << csvEscape(plan.branch) << ','
			 << csvEscape(plan.home_location) << ',' << csvEscape(plan.stance) << ',' << plan.levy_estimate << ','
			 << plan.standing_estimate << '\n';
	}
	return out.str();
}

std::string renderStartForcesCsv(const eu5::World& world)
{
	std::ostringstream out;
	out << "key,tag,branch,location,wartime,units\n";
	for (const auto& force: world.start_forces)
	{
		std::vector<std::string> unit_parts;
		for (const auto& unit: force.units)
		{
			unit_parts.push_back(unit.type + ":" + std::to_string(unit.count));
		}
		out << csvEscape(force.key) << ',' << csvEscape(force.tag) << ',' << csvEscape(force.branch) << ','
			 << csvEscape(force.location) << ',' << (force.wartime ? "yes" : "no") << ',' << csvEscape(common::join(unit_parts, "|"))
			 << '\n';
	}
	return out.str();
}

std::string renderWarsCsv(const eu5::World& world)
{
	std::ostringstream out;
	out << "key,civil_war,name,start_date,action_date,attacker_count,defender_count,target_location\n";
	for (const auto& war: world.wars)
	{
		out << csvEscape(war.key) << ',' << (war.civil_war ? "yes" : "no") << ',' << csvEscape(war.name) << ','
			 << csvEscape(war.start_date) << ',' << csvEscape(war.action_date) << ',' << war.attackers.size() << ','
			 << war.defenders.size() << ',' << csvEscape(war.target_location) << '\n';
	}
	return out.str();
}

}  // namespace

void Eu5Outputter::write(const eu5::World& world,
	 const eu5::WorldFramework& framework,
	 const config::Configuration& configuration,
	 const diagnostics::DiagnosticsReport& diagnostics) const
{
	common::Logger::info("Writing EU5 mod output...");
	common::recreateDirectory(configuration.output_mod_path);

	std::map<std::string, eu5::CountryColorDefinition> fallback_colors;

	common::writeTextFile(configuration.output_mod_path / ".metadata/metadata.json",
		 renderMetadataJson(configuration),
		 common::TextEncoding::Utf8NoBom);
	common::writePlaceholderThumbnailPng(configuration.output_mod_path / ".metadata/thumbnail.png");

	fs::create_directories(configuration.output_mod_path / "loading_screen");
	common::writeTextFile(configuration.output_mod_path / "loading_screen/.keep",
		 "generated by CK3ToEU5\n",
		 common::TextEncoding::Utf8NoBom);

	const auto start_root = configuration.output_mod_path / "main_menu/setup/start";
	common::writeTextFile(start_root / "00_institutions.txt", renderInstitutionManager(world), common::TextEncoding::Utf8NoBom);
	common::writeTextFile(start_root / "03_markets.txt", renderMarkets(world), common::TextEncoding::Utf8NoBom);
	common::writeTextFile(start_root / "04_dynasties.txt", renderDynasties(world), common::TextEncoding::Utf8NoBom);
	common::writeTextFile(start_root / "05_characters.txt", renderCharacters(world), common::TextEncoding::Utf8NoBom);
	common::writeTextFile(start_root / "06_pops.txt", renderPops(world), common::TextEncoding::Utf8NoBom);
	common::writeTextFile(start_root / "07_cities_and_buildings.txt", renderCitiesAndBuildings(world), common::TextEncoding::Utf8NoBom);
	common::writeTextFile(start_root / "09_roads.txt", renderRoads(world), common::TextEncoding::Utf8NoBom);
	common::writeTextFile(start_root / "10_countries.txt", renderCountries(world, configuration), common::TextEncoding::Utf8NoBom);
	common::writeTextFile(start_root / "12_diplomacy.txt", renderDiplomacy(world), common::TextEncoding::Utf8NoBom);
	common::writeTextFile(start_root / "13_religion.txt", renderReligionSetup(), common::TextEncoding::Utf8NoBom);
	common::writeTextFile(start_root / "14_development.txt", renderDevelopment(world), common::TextEncoding::Utf8NoBom);
	common::writeTextFile(start_root / "16_wars.txt", renderWars(world), common::TextEncoding::Utf8NoBom);
	common::writeTextFile(start_root / "18_opinions.txt", renderOpinions(world), common::TextEncoding::Utf8NoBom);
	common::writeTextFile(start_root / "20_rivals.txt", renderRivals(world), common::TextEncoding::Utf8NoBom);

	if (!world.start_forces.empty() || !world.wars.empty())
	{
		common::writeTextFile(configuration.output_mod_path / "in_game/common/on_action/zz_ck3eu5_startup.txt",
			 renderStartupOnAction(world),
			 common::TextEncoding::Utf8Bom);
	}
	if (!world.start_forces.empty())
	{
		common::writeTextFile(configuration.output_mod_path / "in_game/events/zz_ck3eu5_startup.txt",
			 renderStartupEvents(world),
			 common::TextEncoding::Utf8Bom);
	}

	common::writeTextFile(configuration.output_mod_path / "in_game/setup/countries/00_ck3_generated_countries.txt",
		 renderCountryDefinitions(world, framework, fallback_colors),
		 common::TextEncoding::Utf8Bom);
	common::writeTextFile(configuration.output_mod_path / "in_game/common/town_setups/00_ck3_converter_town_setups.txt",
		 renderTownSetups(),
		 common::TextEncoding::Utf8Bom);
	common::writeTextFile(configuration.output_mod_path / "in_game/localization/english/ck3_to_eu5_l_english.yml",
		 renderLocalization(world, framework),
		 common::TextEncoding::Utf8Bom);

	common::writeTextFile(configuration.output_mod_path / "diagnostics/summary.txt", diagnostics.summary(), common::TextEncoding::Utf8NoBom);
	if (configuration.write_debug_snapshots)
	{
		common::writeTextFile(configuration.output_mod_path / "debug/world_summary.txt",
			 renderWorldSummary(world),
			 common::TextEncoding::Utf8NoBom);
		common::writeTextFile(configuration.output_mod_path / "debug/countries.csv",
			 renderCountriesCsv(world),
			 common::TextEncoding::Utf8NoBom);
		common::writeTextFile(configuration.output_mod_path / "debug/characters.csv",
			 renderCharactersCsv(world),
			 common::TextEncoding::Utf8NoBom);
		common::writeTextFile(configuration.output_mod_path / "debug/locations.csv",
			 renderLocationsCsv(world),
			 common::TextEncoding::Utf8NoBom);
		common::writeTextFile(configuration.output_mod_path / "debug/subject_relations.csv",
			 renderSubjectCsv(world),
			 common::TextEncoding::Utf8NoBom);
		common::writeTextFile(configuration.output_mod_path / "debug/markets.csv",
			 renderMarketsCsv(world),
			 common::TextEncoding::Utf8NoBom);
		common::writeTextFile(configuration.output_mod_path / "debug/buildings.csv",
			 renderBuildingsCsv(world),
			 common::TextEncoding::Utf8NoBom);
		common::writeTextFile(configuration.output_mod_path / "debug/force_plans.csv",
			 renderForcePlansCsv(world),
			 common::TextEncoding::Utf8NoBom);
		common::writeTextFile(configuration.output_mod_path / "debug/start_forces.csv",
			 renderStartForcesCsv(world),
			 common::TextEncoding::Utf8NoBom);
		common::writeTextFile(configuration.output_mod_path / "debug/wars.csv",
			 renderWarsCsv(world),
			 common::TextEncoding::Utf8NoBom);
	}
}

}  // namespace ck3eu5::output
