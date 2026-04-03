#include "convert/world_converter.h"

#include "common/logger.h"
#include "common/string_utils.h"
#include "convert/country_context.h"
#include "convert/diplomacy_converter.h"
#include "convert/economy_converter.h"
#include "convert/military_converter.h"
#include "convert/pop_synthesizer.h"
#include "convert/tag_generator.h"
#include "convert/war_converter.h"
#include "mappers/province_matcher.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>

namespace ck3eu5::convert {
namespace {

std::string countryRankFromTitleRank(const ck3::TitleRank rank)
{
	switch (rank)
	{
		case ck3::TitleRank::Empire:
			return "rank_empire";
		case ck3::TitleRank::Kingdom:
			return "rank_kingdom";
		case ck3::TitleRank::Duchy:
			return "rank_duchy";
		case ck3::TitleRank::County:
		case ck3::TitleRank::Barony:
		case ck3::TitleRank::Unknown:
			break;
	}
	return "rank_duchy";
}

std::string makeAdjective(const std::string& display_name)
{
	if (display_name.empty())
	{
		return "Converted";
	}
	if (display_name.ends_with("land"))
	{
		return display_name;
	}
	if (display_name.ends_with("y"))
	{
		return display_name.substr(0, display_name.size() - 1) + "ian";
	}
	return display_name + "ian";
}

std::string makeDynastyKey(const ck3::Character& character, const std::string& dynasty_name)
{
	if (!character.dynasty_house_id.empty())
	{
		return "dyn_house_" + ck3eu5::common::sanitizeIdentifier(character.dynasty_house_id);
	}
	return "dyn_" + ck3eu5::common::sanitizeIdentifier(dynasty_name);
}

std::string makeCharacterKey(const std::string& source_character_id, const std::string& first_name)
{
	return "char_" + ck3eu5::common::sanitizeIdentifier(source_character_id + "_" + first_name);
}

std::string makeLocKey(const std::string& prefix, const std::string& source)
{
	return prefix + "_" + ck3eu5::common::sanitizeIdentifier(source);
}

std::string mappedSourceCultureKey(const ck3::Character& character)
{
	return !character.culture.empty() ? character.culture : character.culture_id;
}

std::string mappedSourceFaithKey(const ck3::Character& character)
{
	return !character.faith.empty() ? character.faith : character.faith_id;
}

std::string determineRank(const ck3::County& county, const eu5::LocationDefinition& definition)
{
	const int cities = static_cast<int>(std::count(county.holdings.begin(), county.holdings.end(), "city"));
	if (county.development >= 18 || cities >= 2 ||
		 (definition.default_rank == "city" && (county.development >= 12 || cities >= 1)))
	{
		return "city";
	}
	if (county.development >= 8 || cities >= 1 || definition.default_rank == "town")
	{
		return "town";
	}
	return definition.default_rank.empty() ? "rural_settlement" : definition.default_rank;
}

int determineControl(const ck3::County& county)
{
	return std::clamp(50 + county.development * 2, 30, 100);
}

double determineProsperity(const ck3::County& county)
{
	return std::clamp(-10.0 + county.development * 2.0, -50.0, 60.0);
}

bool isOwnableLocation(const eu5::LocationDefinition& definition)
{
	const auto topography = ck3eu5::common::toLower(definition.topography);
	if (definition.key.starts_with("connector_") ||
		 ck3eu5::common::toLower(definition.display_name).starts_with("connector"))
	{
		return false;
	}

	return topography.find("wasteland") == std::string::npos && topography.find("coastal_ocean") == std::string::npos &&
			 topography.find("impassable") == std::string::npos && topography.find("inland_sea") == std::string::npos &&
			 topography.find("narrows") == std::string::npos && topography.find("atoll") == std::string::npos &&
			 topography != "ocean";
}

bool isSubjectCandidate(const ck3::World& world,
	 const std::string& character_id,
	 const int direct_counties,
	 const mappers::MapperBundle& mappers,
	 const config::Configuration& configuration)
{
	const auto* character = world.getCharacter(character_id);
	if (!character)
	{
		return false;
	}
	if (character->liege.empty())
	{
		return true;
	}
	const auto primary_title = world.primaryTitleOfCharacter(character_id);
	if (const auto mapped = mappers.mapTitle(primary_title); mapped.has_value())
	{
		return true;
	}
	if (!configuration.prefer_subject_realms)
	{
		return false;
	}
	return world.primaryRankOfCharacter(character_id) >= ck3::TitleRank::Duchy &&
			 direct_counties >= configuration.minimum_subject_counties;
}

std::string firstMappedCapitalLocation(const ck3::World& ck3_world,
	 const ck3::Character& character,
	 const mappers::MapperBundle& mappers)
{
	const auto primary_title = ck3_world.primaryTitleOfCharacter(character.id);
	const auto* title = ck3_world.getTitle(primary_title);
	if (!title || title->capital_county.empty())
	{
		if (!character.realm_capital_province.empty())
		{
			if (const auto* county = ck3_world.findCountyByBaronyProvince(character.realm_capital_province))
			{
				const auto mapped = mappers.mapCountyToLocations(county->key);
				if (!mapped.empty())
				{
					return mapped.front();
				}
			}
		}
		return {};
	}
	const auto mapped = mappers.mapCountyToLocations(title->capital_county);
	if (mapped.empty())
	{
		if (!character.realm_capital_province.empty())
		{
			if (const auto* county = ck3_world.findCountyByBaronyProvince(character.realm_capital_province))
			{
				const auto fallback = mappers.mapCountyToLocations(county->key);
				if (!fallback.empty())
				{
					return fallback.front();
				}
			}
		}
		return {};
	}
	return mapped.front();
}

void appendUnique(std::vector<std::string>& values, const std::string& value)
{
	if (!value.empty() && std::find(values.begin(), values.end(), value) == values.end())
	{
		values.push_back(value);
	}
}

int clampSocietalValue(const int value)
{
	return std::clamp(value, -100, 100);
}

bool containsInsensitive(std::string value, const std::string& needle)
{
	value = ck3eu5::common::toLower(value);
	return value.find(ck3eu5::common::toLower(needle)) != std::string::npos;
}

bool containsAnyInsensitive(const std::string& value, const std::initializer_list<std::string>& needles)
{
	return std::any_of(needles.begin(), needles.end(), [&value](const std::string& needle) {
		return containsInsensitive(value, needle);
	});
}

std::string resolveConvertedCountryCharacter(const ck3::World& ck3_world,
	 const std::string& character_id,
	 const std::set<std::string>& country_characters)
{
	std::string current = character_id;
	while (!current.empty())
	{
		if (country_characters.contains(current))
		{
			return current;
		}
		const auto* character = ck3_world.getCharacter(current);
		if (!character)
		{
			break;
		}
		current = character->liege;
	}
	return {};
}

std::string determineDynastyName(const ck3::Character& character, const std::string& fallback_name)
{
	return character.dynasty.empty() ? fallback_name : character.dynasty;
}

bool isMuslimFaith(const ck3::Faith* faith)
{
	if (!faith)
	{
		return false;
	}

	return containsAnyInsensitive(faith->key + " " + faith->religion + " " + faith->religion_family,
		 {"muslim", "islam", "sunni", "shia", "ashari", "maturidi", "ibadi"});
}

bool isMuslimReligionKey(const std::string& religion)
{
	return containsAnyInsensitive(religion, {"muslim", "islam", "sunni", "shia", "ashari", "maturidi", "ibadi"});
}

bool shouldUseMuslimGovernmentDefaults(const eu5::Country& country, const ck3::Faith* faith)
{
	return isMuslimFaith(faith) || isMuslimReligionKey(country.primary_religion);
}

std::string ensureDynasty(eu5::World& result,
	 const ck3::World& ck3_world,
	 const ck3::Character& character,
	 const std::string& fallback_name,
	 const mappers::MapperBundle& mappers)
{
	const auto dynasty_name = determineDynastyName(character, fallback_name);
	const auto dynasty_key = makeDynastyKey(character, dynasty_name);
	if (!result.dynasties.contains(dynasty_key))
	{
		eu5::Dynasty dynasty;
		dynasty.key = dynasty_key;
		dynasty.loc_name_key = makeLocKey("dynasty", dynasty_name);
		dynasty.home_location = firstMappedCapitalLocation(ck3_world, character, mappers);
		result.dynasties[dynasty_key] = dynasty;
		result.localization[dynasty.loc_name_key] = dynasty_name;
	}
	return dynasty_key;
}

std::string ensureCharacter(eu5::World& result,
	 const ck3::World& ck3_world,
	 const ck3::Character& source_character,
	 const std::string& preferred_tag,
	 const std::string& fallback_name,
	 const mappers::MapperBundle& mappers)
{
	const auto dynasty_name = determineDynastyName(source_character, fallback_name);
	const auto dynasty_key = ensureDynasty(result, ck3_world, source_character, fallback_name, mappers);

	eu5::Character eu5_character;
	eu5_character.key = makeCharacterKey(source_character.id, source_character.first_name);
	eu5_character.first_name_loc_key = makeLocKey("char_first", source_character.id + "_" + source_character.first_name);
	eu5_character.last_name_loc_key =
		 makeLocKey("char_last", source_character.id + "_" + (source_character.last_name.empty() ? dynasty_name : source_character.last_name));
	eu5_character.dynasty_key = dynasty_key;
	eu5_character.culture = mappers.mapCulture(mappedSourceCultureKey(source_character));
	eu5_character.religion = mappers.mapReligion(mappedSourceFaithKey(source_character));
	eu5_character.birth_date = source_character.birth_date;
	eu5_character.birth_location = firstMappedCapitalLocation(ck3_world, source_character, mappers);
	eu5_character.death_date = source_character.death_date;
	if (eu5_character.death_date.empty() && source_character.dead)
	{
		eu5_character.death_date = ck3_world.date;
	}
	eu5_character.adm = source_character.adm;
	eu5_character.dip = source_character.dip;
	eu5_character.mil = source_character.mil;
	eu5_character.female = source_character.female;
	eu5_character.tag = preferred_tag;
	result.characters[eu5_character.key] = eu5_character;
	result.localization[eu5_character.first_name_loc_key] = source_character.first_name.empty() ? fallback_name : source_character.first_name;
	result.localization[eu5_character.last_name_loc_key] = source_character.last_name.empty() ? dynasty_name : source_character.last_name;
	return eu5_character.key;
}

std::vector<std::string> collectTitleNotables(const ck3::Title& title, const ck3::Character& ruler)
{
	std::vector<std::string> related;
	appendUnique(related, ruler.spouse_id);
	for (const auto& heir_id: title.heirs)
	{
		appendUnique(related, heir_id);
	}
	for (const auto& claimant_id: title.claimants)
	{
		appendUnique(related, claimant_id);
	}
	for (const auto& elector_id: title.electors)
	{
		appendUnique(related, elector_id);
	}
	for (const auto& previous_holder_id: title.previous_holders)
	{
		appendUnique(related, previous_holder_id);
	}
	return related;
}

void applyGovernmentMapping(eu5::Country& country, const mappers::GovernmentMapping& mapping)
{
	country.government_type = mapping.eu5_type;
	country.centralization_vs_decentralization = mapping.centralization_vs_decentralization;
	country.traditionalist_vs_innovative = mapping.traditionalist_vs_innovative;
	country.spiritualist_vs_humanist = mapping.spiritualist_vs_humanist;
	country.aristocracy_vs_plutocracy = mapping.aristocracy_vs_plutocracy;
	country.serfdom_vs_free_subjects = mapping.serfdom_vs_free_subjects;
	country.mercantilism_vs_free_trade = mapping.mercantilism_vs_free_trade;
	country.belligerent_vs_conciliatory = mapping.belligerent_vs_conciliatory;
	country.quality_vs_quantity = mapping.quality_vs_quantity;
	country.offensive_vs_defensive = mapping.offensive_vs_defensive;
	country.land_vs_naval = mapping.land_vs_naval;
	country.capital_economy_vs_traditional_economy = mapping.capital_economy_vs_traditional_economy;
	country.individualism_vs_communalism = mapping.individualism_vs_communalism;
	country.outward_vs_inward = mapping.outward_vs_inward;
}

void applyCultureGovernmentBias(eu5::Country& country, const ck3::Culture* culture)
{
	if (!culture)
	{
		return;
	}

	if (culture->ethos == "ethos_bureaucratic")
	{
		country.centralization_vs_decentralization += 20;
		country.capital_economy_vs_traditional_economy += 10;
	}
	else if (culture->ethos == "ethos_bellicose")
	{
		country.belligerent_vs_conciliatory += 20;
		country.offensive_vs_defensive -= 20;
		country.quality_vs_quantity -= 5;
	}
	else if (culture->ethos == "ethos_courtly")
	{
		country.aristocracy_vs_plutocracy -= 10;
		country.belligerent_vs_conciliatory -= 10;
		country.outward_vs_inward -= 10;
	}
	else if (culture->ethos == "ethos_communal")
	{
		country.individualism_vs_communalism -= 30;
		country.outward_vs_inward += 10;
	}
	else if (culture->ethos == "ethos_egalitarian")
	{
		country.serfdom_vs_free_subjects -= 25;
		country.aristocracy_vs_plutocracy += 10;
		country.individualism_vs_communalism += 20;
	}
	else if (culture->ethos == "ethos_spiritual")
	{
		country.spiritualist_vs_humanist -= 25;
		country.traditionalist_vs_innovative += 10;
	}
	else if (culture->ethos == "ethos_stoic")
	{
		country.offensive_vs_defensive += 15;
		country.quality_vs_quantity -= 10;
	}
}

void applyFaithGovernmentBias(eu5::Country& country, const ck3::Faith* faith)
{
	if (!faith)
	{
		return;
	}

	if (containsAnyInsensitive(faith->religion_family, {"abrahamic", "dharmic"}))
	{
		country.spiritualist_vs_humanist -= 10;
	}

	for (const auto& doctrine: faith->doctrines)
	{
		if (containsAnyInsensitive(doctrine, {"plural", "syncret", "tolerance"}))
		{
			country.spiritualist_vs_humanist += 20;
			country.outward_vs_inward -= 10;
		}
		if (containsAnyInsensitive(doctrine, {"warmong", "struggle", "pursuit_of_power"}))
		{
			country.belligerent_vs_conciliatory += 20;
			country.offensive_vs_defensive -= 10;
		}
		if (containsAnyInsensitive(doctrine, {"pacif", "monastic", "monk"}))
		{
			country.belligerent_vs_conciliatory -= 15;
			country.offensive_vs_defensive += 10;
			country.spiritualist_vs_humanist -= 10;
		}
		if (containsAnyInsensitive(doctrine, {"communal"}))
		{
			country.individualism_vs_communalism -= 20;
		}
		if (containsAnyInsensitive(doctrine, {"esoter", "gnostic"}))
		{
			country.traditionalist_vs_innovative -= 10;
			country.spiritualist_vs_humanist += 10;
		}
	}
}

void applyRealmGovernmentBias(eu5::Country& country, const CountryStats& stats, const ck3::Title* source_title)
{
	const double coastal_share =
		 stats.location_count > 0 ? static_cast<double>(stats.coastal_locations) / static_cast<double>(stats.location_count) : 0.0;
	const double urban_share =
		 stats.location_count > 0 ? static_cast<double>(stats.city_locations + stats.town_locations) / static_cast<double>(stats.location_count) : 0.0;

	country.centralization_vs_decentralization += std::min<int>(20, static_cast<int>(stats.subject_count) * 5);
	country.traditionalist_vs_innovative -= static_cast<int>(std::round(stats.average_development / 3.0));
	country.aristocracy_vs_plutocracy += static_cast<int>(std::round((urban_share - 0.25) * 70.0));
	country.serfdom_vs_free_subjects += stats.city_locations == 0 ? 10 : -static_cast<int>(stats.city_locations) * 5;
	country.mercantilism_vs_free_trade -= static_cast<int>(std::round(coastal_share * 45.0));
	country.belligerent_vs_conciliatory += std::min<int>(20, static_cast<int>(stats.subject_count) * 5);
	country.quality_vs_quantity += stats.location_count >= 8 ? 15 : -5;
	country.offensive_vs_defensive += stats.capital_development >= 14 ? -10 : 10;
	country.land_vs_naval += 10 - static_cast<int>(std::round(coastal_share * 50.0));
	country.capital_economy_vs_traditional_economy += static_cast<int>(std::round(stats.capital_development * 2.5)) - 15;
	country.individualism_vs_communalism += static_cast<int>(std::round((urban_share - 0.2) * 60.0));
	country.outward_vs_inward -= static_cast<int>(std::round(coastal_share * 40.0));
	country.outward_vs_inward -= static_cast<int>((country.accepted_cultures.size() + country.tolerated_cultures.size()) * 6);

	if (source_title)
	{
		country.belligerent_vs_conciliatory += std::min<int>(15, static_cast<int>(source_title->claimants.size()) * 3);
		country.centralization_vs_decentralization -= std::min<int>(20, static_cast<int>(source_title->electors.size()) * 5);
	}
}

std::string determineMarriageLaw(const eu5::Country& country,
	 const ck3::Character& character,
	 const ck3::Faith* faith,
	 const ck3::Culture* culture)
{
	if (country.government_type == "theocracy" && character.spouse_id.empty())
	{
		return "celibacy";
	}
	if (shouldUseMuslimGovernmentDefaults(country, faith))
	{
		return "muslim_marriage";
	}
	if (faith)
	{
		for (const auto& doctrine: faith->doctrines)
		{
			if (containsAnyInsensitive(doctrine, {"polyg", "concubin"}))
			{
				return "polygyny";
			}
		}
	}
	if (culture && containsAnyInsensitive(culture->language, {"chinese", "korean", "japanese", "vietnamese"}))
	{
		return "dishu_system";
	}
	return "monogamous_marriage";
}

std::string determineParliamentType(const eu5::Country& country, const ck3::Faith* faith)
{
	if (country.government_type == "tribe" || country.government_type == "steppe_horde")
	{
		return "assembly";
	}
	if (shouldUseMuslimGovernmentDefaults(country, faith))
	{
		return "council";
	}
	return "estate_parliament";
}

std::string determineHeirSelection(const eu5::Country& country,
	 const ck3::Character& ruler,
	 const ck3::Title* source_title,
	 const ck3::Culture* culture,
	 const CountryStats& stats)
{
	if (country.government_type == "republic")
	{
		return stats.city_locations >= 2 || country.mercantilism_vs_free_trade <= -20 ? "republic_4_year_terms" : "oligarchic_elective";
	}
	if (country.government_type == "theocracy")
	{
		return country.belligerent_vs_conciliatory >= 15 ? "grandmaster_elective" : "theocratic_elective";
	}
	if (country.government_type == "tribe" || country.government_type == "steppe_horde")
	{
		return "tribal_oldest_male";
	}
	if (culture && culture->ethos == "ethos_egalitarian")
	{
		return "absolute_cognatic_primogeniture";
	}
	if (source_title && !source_title->electors.empty())
	{
		return country.centralization_vs_decentralization <= 10 ? "partition_inheritance" : "semi_salic_law";
	}
	if (country.centralization_vs_decentralization <= 0 && stats.subject_count > 0)
	{
		return "partition_inheritance";
	}
	if (ruler.female)
	{
		return "cognatic_primogeniture";
	}
	return "cognatic_primogeniture";
}

std::string determineHeirReligionLaw(const eu5::Country& country)
{
	if (country.spiritualist_vs_humanist >= 30)
	{
		return "heir_any_religion";
	}
	if (country.spiritualist_vs_humanist >= 5 || country.accepted_cultures.size() + country.tolerated_cultures.size() >= 3)
	{
		return "heir_same_religion_group";
	}
	return "heir_same_religion";
}

std::string determineLegalCodeLaw(const eu5::Country& country, const ck3::Faith* faith)
{
	return shouldUseMuslimGovernmentDefaults(country, faith) ? "sharia_law_policy" : "civil_law_policy";
}

std::string determineFeudalDeJureLaw(const eu5::Country& country)
{
	return country.outward_vs_inward <= -15 ? "by_blood" : "by_tradition";
}

std::string determineMedievalLevyLaw(const eu5::Country& country)
{
	if (country.quality_vs_quantity >= 15 || country.serfdom_vs_free_subjects <= -10)
	{
		return "peasant_levies";
	}
	if (country.aristocracy_vs_plutocracy <= -10)
	{
		return "noble_levies";
	}
	return "all_cultures";
}

void addGovernmentReforms(eu5::Country& country, const ck3::Faith* faith, const CountryStats& stats)
{
	if (country.government_type == "monarchy")
	{
		appendUnique(country.reforms, country.centralization_vs_decentralization >= 55 ? "autocracy" : "feudal_nobility");
		return;
	}
	if (country.government_type == "republic")
	{
		if (country.mercantilism_vs_free_trade <= -20 || stats.coastal_locations * 2 >= stats.location_count)
		{
			appendUnique(country.reforms, "merchant_republic");
		}
		else if (country.aristocracy_vs_plutocracy <= -15)
		{
			appendUnique(country.reforms, "noble_elite");
		}
		else
		{
			appendUnique(country.reforms, "sortition");
		}
		return;
	}
	if (country.government_type == "theocracy")
	{
		if (faith && containsInsensitive(faith->key, "catholic") && country.belligerent_vs_conciliatory >= 15)
		{
			appendUnique(country.reforms, "military_order_reform");
		}
		else if (country.country_rank == "rank_duchy" || country.country_rank == "rank_kingdom")
		{
			appendUnique(country.reforms, "prince_bishopric_reform");
		}
		else
		{
			appendUnique(country.reforms, "abbey_reform");
		}
	}
}

void addGovernmentPrivileges(eu5::Country& country, const ck3::Faith* faith, const CountryStats& stats)
{
	if (country.aristocracy_vs_plutocracy <= -10)
	{
		appendUnique(country.privileges, "nobles_land_rights");
		appendUnique(country.privileges, "noble_marriage_rights");
	}
	if (country.offensive_vs_defensive >= 0)
	{
		appendUnique(country.privileges, "noble_fortification_licenses");
	}
	if (country.spiritualist_vs_humanist <= -10)
	{
		appendUnique(country.privileges, "clergy_literacy_rights");
		appendUnique(country.privileges, shouldUseMuslimGovernmentDefaults(country, faith) ? "clerical_advisory_council" : "clergy_enforced_unity");
	}
	if (stats.city_locations > 0)
	{
		appendUnique(country.privileges, "market_fairs");
	}
	if (stats.coastal_locations > 0)
	{
		appendUnique(country.privileges, "shipwright_contracts");
	}
	if (stats.city_locations >= 2)
	{
		appendUnique(country.privileges, "formal_guilds");
	}
	if (country.serfdom_vs_free_subjects <= -10)
	{
		appendUnique(country.privileges, "peasants_free_peasantry");
		appendUnique(country.privileges, "access_to_royal_and_ecclesiastical_courts");
	}
	if (country.belligerent_vs_conciliatory >= 10 || country.government_type == "tribe")
	{
		appendUnique(country.privileges, "peasants_allowed_weapons_privilege");
	}
	if (shouldUseMuslimGovernmentDefaults(country, faith))
	{
		appendUnique(country.privileges, "clergy_land_rights");
		appendUnique(country.privileges, "dhimmi_promote_tolerance");
	}
}

void clampGovernmentProfile(eu5::Country& country)
{
	country.centralization_vs_decentralization = clampSocietalValue(country.centralization_vs_decentralization);
	country.traditionalist_vs_innovative = clampSocietalValue(country.traditionalist_vs_innovative);
	country.spiritualist_vs_humanist = clampSocietalValue(country.spiritualist_vs_humanist);
	country.aristocracy_vs_plutocracy = clampSocietalValue(country.aristocracy_vs_plutocracy);
	country.serfdom_vs_free_subjects = clampSocietalValue(country.serfdom_vs_free_subjects);
	country.mercantilism_vs_free_trade = clampSocietalValue(country.mercantilism_vs_free_trade);
	country.belligerent_vs_conciliatory = clampSocietalValue(country.belligerent_vs_conciliatory);
	country.quality_vs_quantity = clampSocietalValue(country.quality_vs_quantity);
	country.offensive_vs_defensive = clampSocietalValue(country.offensive_vs_defensive);
	country.land_vs_naval = clampSocietalValue(country.land_vs_naval);
	country.capital_economy_vs_traditional_economy = clampSocietalValue(country.capital_economy_vs_traditional_economy);
	country.individualism_vs_communalism = clampSocietalValue(country.individualism_vs_communalism);
	country.outward_vs_inward = clampSocietalValue(country.outward_vs_inward);
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

}  // namespace

eu5::World WorldConverter::convert(const ck3::World& ck3_world,
	 const eu5::WorldFramework& framework,
	 const mappers::MapperBundle& mappers,
	 const config::Configuration& configuration,
	 diagnostics::DiagnosticsReport& diagnostics) const
{
	eu5::World result;
	result.date = ck3_world.date;

	std::unordered_map<std::string, int> direct_owned_counties;
	for (const auto& [county_key, county]: ck3_world.counties)
	{
		if (!county.owner_id.empty())
		{
			++direct_owned_counties[county.owner_id];
		}
	}

	std::set<std::string> country_characters;
	for (const auto& [character_id, count]: direct_owned_counties)
	{
		if (count <= 0)
		{
			continue;
		}
		if (isSubjectCandidate(ck3_world, character_id, count, mappers, configuration))
		{
			country_characters.insert(character_id);
		}
	}

	std::unordered_map<std::string, std::string> county_to_country_character;
	for (const auto& [county_key, county]: ck3_world.counties)
	{
		std::string current = county.owner_id;
		std::string chosen;
		while (!current.empty())
		{
			if (country_characters.contains(current))
			{
				chosen = current;
				break;
			}
			const auto* character = ck3_world.getCharacter(current);
			if (!character)
			{
				break;
			}
			current = character->liege;
		}
		if (chosen.empty() && !county.top_liege_id.empty())
		{
			chosen = county.top_liege_id;
		}
		if (chosen.empty())
		{
			chosen = county.owner_id;
		}
		if (chosen.empty())
		{
			diagnostics.error("COUNTY_NO_OWNER", "County " + county_key + " has no resolvable owner.");
			continue;
		}
		country_characters.insert(chosen);
		county_to_country_character[county_key] = chosen;
	}

	auto reserved_tags = mappers.reserved_tags;
	for (const auto& [tag, _]: framework.colors)
	{
		reserved_tags.insert(tag);
	}
	TagGenerator tag_generator(std::move(reserved_tags));
	std::unordered_map<std::string, std::string> character_to_tag;

	for (const auto& character_id: country_characters)
	{
		const auto* character = ck3_world.getCharacter(character_id);
		if (!character)
		{
			continue;
		}
		const auto source_title = ck3_world.primaryTitleOfCharacter(character_id);
		const auto title_mapping = mappers.mapTitle(source_title);

		eu5::Country country;
		country.source_character_id = character_id;
		country.source_title_key = source_title;
		country.tag = title_mapping && !title_mapping->eu5_tag.empty() ? title_mapping->eu5_tag :
																	tag_generator.getOrCreate(source_title.empty() ? ("char_" + character_id) : source_title);
		country.display_name = title_mapping && !title_mapping->display_name.empty() ? title_mapping->display_name :
							([&]() {
								if (const auto* title = ck3_world.getTitle(source_title))
								{
									return title->display_name;
								}
								if (!character->primary_title.empty())
								{
									if (const auto* primary_title = ck3_world.getTitle(character->primary_title))
									{
										return primary_title->display_name;
									}
								}
								return character->first_name.empty() ? ("Realm " + character_id) : (character->first_name + "'s Realm");
							})();
		country.adjective = title_mapping && !title_mapping->adjective.empty() ? title_mapping->adjective : makeAdjective(country.display_name);
		country.country_rank = title_mapping && !title_mapping->country_rank.empty() ? title_mapping->country_rank :
																			 countryRankFromTitleRank(ck3_world.primaryRankOfCharacter(character_id));
		country.starting_technology_level = title_mapping && title_mapping->technology_level >= 0 ?
														 title_mapping->technology_level :
														 configuration.default_technology_level;
		country.gold = configuration.default_gold + static_cast<int>(std::round(character->gold));
		country.prestige = 20 + direct_owned_counties[character_id] * 5;
		if (const auto* source_title_definition = ck3_world.getTitle(source_title))
		{
			country.prestige += std::min<int>(15,
				 static_cast<int>(source_title_definition->heirs.size()) +
						 static_cast<int>(source_title_definition->claimants.size()) * 2 +
						 static_cast<int>(source_title_definition->electors.size()) * 2 +
						 static_cast<int>(source_title_definition->previous_holders.size()));
		}

		const auto government_source = !character->government.empty() ? character->government :
																			 (ck3_world.getTitle(source_title) ? ck3_world.getTitle(source_title)->government : "");
		const auto government = mappers.mapGovernment(government_source);
		applyGovernmentMapping(country, government);
		result.localization[country.tag] = country.display_name;
		result.localization[country.tag + "_ADJ"] = country.adjective;

		character_to_tag[character_id] = country.tag;
		result.countries[country.tag] = country;
	}

	for (const auto& character_id: country_characters)
	{
		const auto* character = ck3_world.getCharacter(character_id);
		const auto tag_it = character_to_tag.find(character_id);
		if (!character || tag_it == character_to_tag.end())
		{
			continue;
		}
		auto& country = result.countries.at(tag_it->second);
		country.ruler_character_key = ensureCharacter(result, ck3_world, *character, country.tag, country.display_name, mappers);

		const auto* source_title = ck3_world.getTitle(country.source_title_key);
		if (!source_title)
		{
			continue;
		}

		for (const auto& related_character_id: collectTitleNotables(*source_title, *character))
		{
			if (related_character_id.empty() || related_character_id == character_id)
			{
				continue;
			}
			const auto* related_character = ck3_world.getCharacter(related_character_id);
			if (!related_character)
			{
				continue;
			}
			const auto owning_country_character = resolveConvertedCountryCharacter(ck3_world, related_character_id, country_characters);
			if (!owning_country_character.empty() && owning_country_character != character_id)
			{
				continue;
			}
			ensureCharacter(result, ck3_world, *related_character, country.tag, country.display_name, mappers);
		}

		if (!character->spouse_id.empty())
		{
			if (const auto* spouse = ck3_world.getCharacter(character->spouse_id);
				 spouse && !spouse->dead &&
				 resolveConvertedCountryCharacter(ck3_world, character->spouse_id, country_characters).empty())
			{
				country.consort_character_key = ensureCharacter(result, ck3_world, *spouse, country.tag, country.display_name, mappers);
			}
			else if (spouse && !spouse->dead &&
						 resolveConvertedCountryCharacter(ck3_world, character->spouse_id, country_characters) == character_id)
			{
				country.consort_character_key = ensureCharacter(result, ck3_world, *spouse, country.tag, country.display_name, mappers);
			}
		}

		for (const auto& heir_id: source_title->heirs)
		{
			const auto* heir = ck3_world.getCharacter(heir_id);
			if (!heir || heir->dead)
			{
				continue;
			}
			const auto owning_country_character = resolveConvertedCountryCharacter(ck3_world, heir_id, country_characters);
			if (!owning_country_character.empty() && owning_country_character != character_id)
			{
				continue;
			}
			country.heir_character_key = ensureCharacter(result, ck3_world, *heir, country.tag, country.display_name, mappers);
			break;
		}
	}

	PopSynthesizer pop_synthesizer;
	mappers::ProvinceMatcher province_matcher(framework);
	std::unordered_map<std::string, std::string> county_to_primary_location;
	std::map<std::string, size_t> automatic_match_counts;

	for (const auto& [county_key, county]: ck3_world.counties)
	{
		const auto owner_character_it = county_to_country_character.find(county_key);
		if (owner_character_it == county_to_country_character.end())
		{
			continue;
		}
		const auto owner_tag_it = character_to_tag.find(owner_character_it->second);
		if (owner_tag_it == character_to_tag.end())
		{
			continue;
		}
		const auto owner_tag = owner_tag_it->second;
		auto mapped_locations = mappers.mapCountyToLocations(county_key);
		if (mapped_locations.empty())
		{
			mapped_locations = framework.locationsForProvince(county.province_key);
		}
		if (mapped_locations.empty())
		{
			if (const auto automatic_match = province_matcher.match(county); automatic_match.has_value())
			{
				mapped_locations = automatic_match->eu5_locations;
				++automatic_match_counts[automatic_match->source];
			}
		}
		std::vector<std::string> ownable_locations;
		for (const auto& location_key: mapped_locations)
		{
			const auto* definition = framework.getLocation(location_key);
			if (definition && isOwnableLocation(*definition))
			{
				ownable_locations.push_back(location_key);
			}
		}
		mapped_locations = std::move(ownable_locations);
		if (mapped_locations.empty())
		{
			diagnostics.error("COUNTY_UNMAPPED", "County " + county_key + " has no ownable EU5 location mapping.");
			continue;
		}
		county_to_primary_location[county_key] = mapped_locations.front();

		const auto* direct_owner = ck3_world.getCharacter(county.owner_id);
		const auto pops = pop_synthesizer.synthesize(county, direct_owner, mappers, mapped_locations.size());

		for (const auto& location_key: mapped_locations)
		{
			const auto* definition = framework.getLocation(location_key);
			if (!definition)
			{
				diagnostics.error("LOCATION_UNKNOWN", "Mapped EU5 location " + location_key + " for county " + county_key + " is absent from the framework.");
				continue;
			}

			auto& location = result.locations[location_key];
			location.key = definition->key;
			location.owner_tag = owner_tag;
			location.culture = mappers.mapCulture(county.culture);
			location.religion = mappers.mapReligion(county.faith);
			location.rank = determineRank(county, *definition);
			location.raw_good = definition->raw_good;
			location.region = definition->region;
			location.area = definition->area;
			location.province_definition = definition->province_definition;
			location.control = determineControl(county);
			location.prosperity = determineProsperity(county);
			location.development += static_cast<double>(county.development) / static_cast<double>(mapped_locations.size());
			location.has_feudalism = true;

			if (!definition->town_setup.empty())
			{
				location.town_setup = definition->town_setup;
			}
			else if (location.rank == "city")
			{
				location.town_setup = "ck3_converter_city";
			}
			else if (location.rank == "town")
			{
				location.town_setup = "ck3_converter_town";
			}

			for (const auto& pop: pops)
			{
				location.pops.push_back(pop);
			}

			auto& country = result.countries[owner_tag];
			country.owned_core_locations.insert(location_key);
			if (!location.region.empty())
			{
				country.discovered_regions.insert(location.region);
			}
		}
	}

	for (auto& [tag, country]: result.countries)
	{
		if (country.owned_core_locations.empty())
		{
			diagnostics.warning("COUNTRY_EMPTY", "Country " + tag + " has no locations after conversion.");
			continue;
		}

		std::string preferred_capital;
		if (const auto* source_character = ck3_world.getCharacter(country.source_character_id))
		{
			preferred_capital = firstMappedCapitalLocation(ck3_world, *source_character, mappers);
		}

		if (!preferred_capital.empty() && country.owned_core_locations.contains(preferred_capital))
		{
			country.capital_location = preferred_capital;
		}
		else
		{
			double best_development = -1.0;
			for (const auto& location_key: country.owned_core_locations)
			{
				const auto& location = result.locations.at(location_key);
				if (location.development > best_development)
				{
					best_development = location.development;
					country.capital_location = location_key;
				}
			}
		}

		std::map<std::string, double> culture_weights;
		std::map<std::string, double> religion_weights;
		double total_population = 0.0;

		for (const auto& location_key: country.owned_core_locations)
		{
			auto& location = result.locations.at(location_key);
			if (location_key == country.capital_location && location.rank == "town")
			{
				location.town_setup = "ck3_converter_capital_town";
			}
			else if (location_key == country.capital_location && location.rank == "city")
			{
				location.town_setup = "ck3_converter_capital_city";
			}

			for (const auto& pop: location.pops)
			{
				total_population += pop.size;
				culture_weights[pop.culture] += pop.size;
				religion_weights[pop.religion] += pop.size;
			}
		}

		auto max_culture = std::max_element(culture_weights.begin(), culture_weights.end(), [](const auto& lhs, const auto& rhs) {
			return lhs.second < rhs.second;
		});
		if (max_culture != culture_weights.end())
		{
			country.primary_culture = max_culture->first;
		}
		auto max_religion = std::max_element(religion_weights.begin(), religion_weights.end(), [](const auto& lhs, const auto& rhs) {
			return lhs.second < rhs.second;
		});
		if (max_religion != religion_weights.end())
		{
			country.primary_religion = max_religion->first;
		}

		for (const auto& [culture, weight]: culture_weights)
		{
			if (culture == country.primary_culture || total_population <= 0.0)
			{
				continue;
			}
			const double share = weight / total_population;
			if (share >= 0.25)
			{
				country.accepted_cultures.insert(culture);
			}
			else if (share >= 0.10)
			{
				country.tolerated_cultures.insert(culture);
			}
		}
	}

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
		const auto subject_tag = character_to_tag[character_id];
		const auto liege_tag = character_to_tag[parent];
		if (subject_tag == liege_tag || subject_tag.empty() || liege_tag.empty())
		{
			continue;
		}
		result.subject_relations.push_back(eu5::SubjectRelation{.liege_tag = liege_tag,
			 .subject_tag = subject_tag,
			 .subject_type = determineSubjectType(result.countries.at(liege_tag), result.countries.at(subject_tag)),
			 .subject_military_stance = "",
			 .start_date = result.date});
	}

	for (auto& [tag, country]: result.countries)
	{
		const auto* source_character = ck3_world.getCharacter(country.source_character_id);
		const auto* source_title = ck3_world.getTitle(country.source_title_key);
		if (!source_character)
		{
			continue;
		}

		const auto government_source = !source_character->government.empty() ? source_character->government :
																			 (source_title ? source_title->government : "");
		const auto government = mappers.mapGovernment(government_source);
		applyGovernmentMapping(country, government);

		const auto* source_culture = ck3_world.getCulture(source_character->culture_id);
		const auto* source_faith = ck3_world.getFaith(source_character->faith_id);
		const auto stats = measureCountryStats(country, result, framework);

		applyCultureGovernmentBias(country, source_culture);
		applyFaithGovernmentBias(country, source_faith);
		applyRealmGovernmentBias(country, stats, source_title);
		clampGovernmentProfile(country);
		country.parliament_type = determineParliamentType(country, source_faith);

		const int rank_bonus = country.country_rank == "rank_empire" ? 20 : (country.country_rank == "rank_kingdom" ? 10 : 0);
		country.government_power = std::clamp(
				45 + rank_bonus + country.centralization_vs_decentralization / 4 +
						static_cast<int>(stats.subject_count) * 3 - static_cast<int>(source_title ? source_title->claimants.size() * 2 : 0),
				20,
				100);

		country.gold = configuration.default_gold + static_cast<int>(std::lround(source_character->gold)) +
					 static_cast<int>(std::lround(stats.total_development * 4.0)) + static_cast<int>(stats.city_locations) * 18 +
					 static_cast<int>(stats.town_locations) * 8;
		country.prestige += rank_bonus + static_cast<int>(std::lround(stats.average_development)) +
						 static_cast<int>(stats.subject_count) * 5;

		if (const auto title_mapping = mappers.mapTitle(country.source_title_key);
			 !(title_mapping && title_mapping->technology_level >= 0))
		{
			const int computed_technology = configuration.default_technology_level +
											 static_cast<int>(stats.average_development / 10.0) + (stats.city_locations > 0 ? 1 : 0);
			country.starting_technology_level = std::clamp(computed_technology, 1, 6);
		}

		country.heir_selection = determineHeirSelection(country, *source_character, source_title, source_culture, stats);
		country.laws.clear();
		country.reforms.clear();
		country.privileges.clear();
		country.laws["marriage_law"] = determineMarriageLaw(country, *source_character, source_faith, source_culture);
		country.laws["heir_religion_law"] = determineHeirReligionLaw(country);
		country.laws["legal_code_law"] = determineLegalCodeLaw(country, source_faith);
		country.laws["censorship"] = "limited_censorship";
		country.laws["education_masses_law"] = "basic_religious_education";
		country.laws["administrative_system"] = "feudal_administration";
		country.laws["cultural_traditions_law"] = "martial_society";
		country.laws["distribution_of_power_law"] = "dop_traditional_distribution_of_power";
		country.laws["immigration_law"] = "open_borders_law";
		country.laws["mining_law"] = "nobles_mining_law";
		country.laws["coin_laws"] = "gold_and_silver_coins";
		if (country.government_type == "monarchy")
		{
			country.laws["medieval_levy_law"] = determineMedievalLevyLaw(country);
			country.laws["feudal_de_jure_law"] = determineFeudalDeJureLaw(country);
			country.laws["royal_court_customs_law"] = "aristocratic_court_policy";
			if (stats.coastal_locations > 0)
			{
				country.laws["maritime_law"] = "protect_trade_routes";
				country.laws["piracy_law"] = "anti_piracy_policy";
			}
			if (shouldUseMuslimGovernmentDefaults(country, source_faith))
			{
				country.laws["iqta_law"] = "efficient_tax_farming";
			}
		}
		addGovernmentReforms(country, source_faith, stats);
		addGovernmentPrivileges(country, source_faith, stats);
	}

	for (auto& relation: result.subject_relations)
	{
		const auto liege_it = result.countries.find(relation.liege_tag);
		const auto subject_it = result.countries.find(relation.subject_tag);
		if (liege_it == result.countries.end() || subject_it == result.countries.end())
		{
			continue;
		}
		relation.subject_type = determineSubjectType(liege_it->second, subject_it->second);
	}

	const auto border_graph = buildBorderGraph(ck3_world, county_to_primary_location, county_to_country_character, character_to_tag);
	DiplomacyConverter diplomacy_converter;
	diplomacy_converter.convert(ck3_world, border_graph, country_characters, character_to_tag, result);

	EconomyConverter economy_converter;
	economy_converter.convert(border_graph, framework, result);

	MilitaryConverter military_converter;
	military_converter.convert(ck3_world, border_graph, framework, result);

	WarConverter war_converter;
	war_converter.convert(ck3_world, country_characters, character_to_tag, county_to_primary_location, result);

	diagnostics.info("CONVERSION_COUNTRIES", "Generated " + std::to_string(result.countries.size()) + " EU5 countries.");
	diagnostics.info("CONVERSION_LOCATIONS", "Generated " + std::to_string(result.locations.size()) + " EU5 locations.");
	if (!automatic_match_counts.empty())
	{
		std::ostringstream message;
		bool first = true;
		for (const auto& [source, count]: automatic_match_counts)
		{
			if (!first)
			{
				message << ", ";
			}
			message << source << '=' << count;
			first = false;
		}
		diagnostics.info("AUTO_PROVINCE_MATCHES", "Automatic county matches applied: " + message.str());
	}
	return result;
}

}  // namespace ck3eu5::convert
