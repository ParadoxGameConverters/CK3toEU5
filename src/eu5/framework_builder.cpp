#include "eu5/framework_builder.h"

#include "common/csv_reader.h"
#include "common/string_utils.h"
#include "eu5/installed_data_extractor.h"

namespace ck3eu5::eu5 {
namespace {

void mergeLocationRow(WorldFramework& framework, const common::CsvRow& row)
{
	LocationDefinition location;
	location.key = row.get("location_key");
	if (location.key.empty())
	{
		return;
	}

	auto existing = framework.locations.contains(location.key) ? framework.locations.at(location.key) : LocationDefinition{};
	if (existing.key.empty())
	{
		existing.key = location.key;
	}

	if (const auto province_definition = row.get("province_definition"); !province_definition.empty())
	{
		existing.province_definition = province_definition;
	}
	existing.display_name = row.get("display_name", existing.display_name.empty() ? location.key : existing.display_name);
	existing.raw_good = row.get("raw_good", existing.raw_good.empty() ? "grain" : existing.raw_good);
	existing.region = row.get("region", existing.region);
	existing.area = row.get("area", existing.area);
	existing.climate = row.get("climate", existing.climate);
	existing.topography = row.get("topography", existing.topography);
	existing.default_rank = row.get("default_rank", existing.default_rank.empty() ? "rural_settlement" : existing.default_rank);
	existing.town_setup = row.get("town_setup", existing.town_setup);
	existing.coastal = common::parseBool(row.get("coastal", existing.coastal ? "yes" : "no"), existing.coastal);
	existing.has_port = common::parseBool(row.get("has_port", existing.has_port ? "yes" : "no"), existing.has_port);
	if (existing.has_port)
	{
		existing.coastal = true;
	}

	framework.locations[location.key] = existing;
}

void rebuildProvinceIndex(WorldFramework& framework)
{
	framework.province_to_locations.clear();
	for (const auto& [key, location]: framework.locations)
	{
		if (!location.province_definition.empty())
		{
			framework.province_to_locations[location.province_definition].push_back(key);
		}
	}
}

void mergeLocationCsv(const std::filesystem::path& path,
	 WorldFramework& framework,
	 diagnostics::DiagnosticsReport& diagnostics)
{
	for (const auto& row: common::CsvReader::readFile(path))
	{
		if (row.get("location_key").empty())
		{
			diagnostics.error("EU5_FRAMEWORK_LOCATION_KEY", "A framework row is missing location_key.");
			continue;
		}
		mergeLocationRow(framework, row);
	}
	rebuildProvinceIndex(framework);
}

void mergeColorCsv(const std::filesystem::path& path, WorldFramework& framework)
{
	for (const auto& row: common::CsvReader::readFile(path))
	{
		CountryColorDefinition definition;
		definition.tag = row.get("tag");
		if (definition.tag.empty())
		{
			continue;
		}

		auto existing = framework.colors.contains(definition.tag) ? framework.colors.at(definition.tag) : CountryColorDefinition{};
		existing.tag = definition.tag;
		existing.color = row.get("color", existing.color);
		existing.color2 = row.get("color2", existing.color2);
		existing.color3 = row.get("color3", existing.color3);
		existing.unit_color0 = row.get("unit_color0", existing.unit_color0);
		existing.unit_color1 = row.get("unit_color1", existing.unit_color1);
		existing.unit_color2 = row.get("unit_color2", existing.unit_color2);
		existing.description_category = row.get("description_category", existing.description_category);
		existing.difficulty = common::parseInt(row.get("difficulty", std::to_string(existing.difficulty))).value_or(existing.difficulty);
		framework.colors[definition.tag] = existing;
	}
}

}  // namespace

WorldFramework WorldFrameworkBuilder::load(const config::Configuration& configuration,
	 diagnostics::DiagnosticsReport& diagnostics) const
{
	WorldFramework framework;

	if (!configuration.eu5_game_path.empty())
	{
		InstalledDataExtractor extractor;
		framework = extractor.extract(configuration.eu5_game_path, diagnostics);
	}

	mergeLocationCsv(configuration.location_framework_path, framework, diagnostics);
	mergeColorCsv(configuration.country_colors_path, framework);
	return framework;
}

}  // namespace ck3eu5::eu5
