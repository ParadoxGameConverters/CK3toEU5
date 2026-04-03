#include "mappers/mapper_bundle.h"

#include "common/csv_reader.h"
#include "common/string_utils.h"

#include <algorithm>

namespace ck3eu5::mappers {

std::vector<std::string> MapperBundle::mapCountyToLocations(const std::string& county_key) const
{
	const auto it = province_mappings.find(county_key);
	return it == province_mappings.end() ? std::vector<std::string>{} : it->second;
}

std::optional<TitleMapping> MapperBundle::mapTitle(const std::string& title_key) const
{
	const auto it = title_mappings.find(title_key);
	if (it == title_mappings.end())
	{
		return std::nullopt;
	}
	return it->second;
}

std::string MapperBundle::mapCulture(const std::string& culture_key) const
{
	if (culture_key.empty())
	{
		return "generic";
	}
	const auto it = culture_mappings.find(culture_key);
	return it == culture_mappings.end() ? common::sanitizeIdentifier(culture_key) : it->second;
}

std::string MapperBundle::mapReligion(const std::string& religion_key) const
{
	if (religion_key.empty())
	{
		return "catholic";
	}
	const auto it = religion_mappings.find(religion_key);
	return it == religion_mappings.end() ? common::sanitizeIdentifier(religion_key) : it->second;
}

GovernmentMapping MapperBundle::mapGovernment(const std::string& government_key) const
{
	const auto it = government_mappings.find(government_key);
	if (it != government_mappings.end())
	{
		return it->second;
	}

	if (government_key.ends_with("_government"))
	{
		const auto stripped = government_key.substr(0, government_key.size() - std::string("_government").size());
		const auto stripped_it = government_mappings.find(stripped);
		if (stripped_it != government_mappings.end())
		{
			return stripped_it->second;
		}
	}

	return GovernmentMapping{};
}

MapperBundle MapperBundleBuilder::load(const config::Configuration& configuration,
	 diagnostics::DiagnosticsReport& diagnostics) const
{
	MapperBundle bundle;

	for (const auto& row: common::CsvReader::readFile(configuration.province_mappings_path))
	{
		const auto county_key = row.get("ck3_county");
		if (county_key.empty())
		{
			continue;
		}
		auto locations = common::split(row.get("eu5_locations"), '|');
		locations.erase(std::remove_if(locations.begin(), locations.end(), [](const std::string& value) {
			return value.empty();
		}), locations.end());
		if (locations.empty())
		{
			diagnostics.warning("MAPPER_EMPTY_PROVINCE_MAPPING", "County " + county_key + " has no mapped EU5 locations.");
		}
		bundle.province_mappings[county_key] = std::move(locations);
	}

	for (const auto& row: common::CsvReader::readFile(configuration.title_mappings_path))
	{
		TitleMapping mapping;
		mapping.ck3_title = row.get("ck3_title");
		if (mapping.ck3_title.empty())
		{
			continue;
		}
		mapping.eu5_tag = common::toUpper(row.get("eu5_tag"));
		mapping.display_name = row.get("display_name");
		mapping.adjective = row.get("adjective");
		mapping.country_rank = row.get("country_rank");
		mapping.technology_level = common::parseInt(row.get("technology_level")).value_or(-1);
		if (!mapping.eu5_tag.empty())
		{
			bundle.reserved_tags.insert(mapping.eu5_tag);
		}
		bundle.title_mappings[mapping.ck3_title] = mapping;
	}

	for (const auto& row: common::CsvReader::readFile(configuration.culture_mappings_path))
	{
		const auto source = row.get("ck3_culture");
		if (!source.empty())
		{
			bundle.culture_mappings[source] = row.get("eu5_culture", common::sanitizeIdentifier(source));
		}
	}

	for (const auto& row: common::CsvReader::readFile(configuration.religion_mappings_path))
	{
		const auto source = row.get("ck3_faith");
		if (!source.empty())
		{
			bundle.religion_mappings[source] = row.get("eu5_religion", common::sanitizeIdentifier(source));
		}
	}

	for (const auto& row: common::CsvReader::readFile(configuration.government_mappings_path))
	{
		GovernmentMapping mapping;
		mapping.ck3_government = row.get("ck3_government");
		if (mapping.ck3_government.empty())
		{
			continue;
		}
		mapping.eu5_type = row.get("eu5_type", "monarchy");
		mapping.centralization_vs_decentralization =
				common::parseInt(row.get("centralization_vs_decentralization")).value_or(0);
		mapping.traditionalist_vs_innovative =
				common::parseInt(row.get("traditionalist_vs_innovative")).value_or(0);
		mapping.spiritualist_vs_humanist = common::parseInt(row.get("spiritualist_vs_humanist")).value_or(0);
		mapping.aristocracy_vs_plutocracy = common::parseInt(row.get("aristocracy_vs_plutocracy")).value_or(0);
		mapping.serfdom_vs_free_subjects = common::parseInt(row.get("serfdom_vs_free_subjects")).value_or(0);
		mapping.mercantilism_vs_free_trade = common::parseInt(row.get("mercantilism_vs_free_trade")).value_or(0);
		mapping.belligerent_vs_conciliatory = common::parseInt(row.get("belligerent_vs_conciliatory")).value_or(0);
		mapping.quality_vs_quantity = common::parseInt(row.get("quality_vs_quantity")).value_or(0);
		mapping.offensive_vs_defensive = common::parseInt(row.get("offensive_vs_defensive")).value_or(0);
		mapping.land_vs_naval = common::parseInt(row.get("land_vs_naval")).value_or(0);
		mapping.capital_economy_vs_traditional_economy =
				common::parseInt(row.get("capital_economy_vs_traditional_economy")).value_or(0);
		mapping.individualism_vs_communalism = common::parseInt(row.get("individualism_vs_communalism")).value_or(0);
		mapping.outward_vs_inward = common::parseInt(row.get("outward_vs_inward")).value_or(0);
		bundle.government_mappings[mapping.ck3_government] = mapping;
	}

	return bundle;
}

}  // namespace ck3eu5::mappers
