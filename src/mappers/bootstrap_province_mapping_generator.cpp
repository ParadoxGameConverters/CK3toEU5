#include "mappers/bootstrap_province_mapping_generator.h"

#include "common/csv_reader.h"
#include "common/filesystem_utils.h"
#include "common/string_utils.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <sstream>
#include <string_view>

namespace ck3eu5::mappers {
namespace {

std::string csvEscape(std::string_view value)
{
	std::string escaped = "\"";
	for (const char character: value)
	{
		if (character == '"')
		{
			escaped += "\"\"";
		}
		else
		{
			escaped.push_back(character);
		}
	}
	escaped.push_back('"');
	return escaped;
}

std::string stripTitlePrefix(std::string key)
{
	if (key.size() > 2 && key[1] == '_')
	{
		key.erase(0, 2);
	}
	return key;
}

std::string stripTrailingDigits(std::string value)
{
	while (!value.empty() && std::isdigit(static_cast<unsigned char>(value.back())))
	{
		value.pop_back();
	}
	return value;
}

std::string normalizeName(const std::string& value)
{
	return common::sanitizeIdentifier(value);
}

std::vector<std::string> buildKeyVariants(const std::string& title_key)
{
	std::vector<std::string> variants;
	std::set<std::string> seen;

	const auto push_variant = [&](const std::string& raw_value) {
		const auto normalized = common::sanitizeIdentifier(raw_value);
		if (!normalized.empty() && seen.insert(normalized).second)
		{
			variants.push_back(normalized);
		}

		const auto no_digits = common::sanitizeIdentifier(stripTrailingDigits(raw_value));
		if (!no_digits.empty() && seen.insert(no_digits).second)
		{
			variants.push_back(no_digits);
		}
	};

	const auto stripped_key = stripTitlePrefix(title_key);
	push_variant(stripped_key);

	const auto parts = common::split(stripped_key, '_');
	if (parts.size() <= 1)
	{
		return variants;
	}

	for (size_t start = 1; start < parts.size(); ++start)
	{
		std::vector<std::string> suffix(parts.begin() + static_cast<std::ptrdiff_t>(start), parts.end());
		push_variant(common::join(suffix, "_"));
	}

	return variants;
}

struct Eu5LookupIndex
{
	std::map<std::string, std::string> unique_display_names;
};

Eu5LookupIndex buildIndex(const eu5::WorldFramework& framework)
{
	Eu5LookupIndex index;
	for (const auto& [key, location]: framework.locations)
	{
		const auto normalized = normalizeName(location.display_name);
		if (normalized.empty())
		{
			continue;
		}

		const auto [it, inserted] = index.unique_display_names.emplace(normalized, key);
		if (!inserted && it->second != key)
		{
			it->second.clear();
		}
	}
	return index;
}

struct CandidateAccumulator
{
	std::vector<std::string> locations;
	std::vector<std::string> sources;
	std::set<std::string> seen_locations;
	std::set<std::string> seen_sources;
};

bool addLocations(CandidateAccumulator& accumulator,
	 const std::vector<std::string>& locations,
	 const std::string& source)
{
	bool added_any = false;
	for (const auto& location: locations)
	{
		if (location.empty())
		{
			continue;
		}
		if (accumulator.seen_locations.insert(location).second)
		{
			accumulator.locations.push_back(location);
			added_any = true;
		}
	}
	if (added_any && accumulator.seen_sources.insert(source).second)
	{
		accumulator.sources.push_back(source);
	}
	return added_any;
}

bool addLocation(CandidateAccumulator& accumulator, const std::string& location, const std::string& source)
{
	return addLocations(accumulator, {location}, source);
}

bool addMatchesForKey(CandidateAccumulator& accumulator,
	 const std::string& title_key,
	 const eu5::WorldFramework& framework,
	 const std::string& location_source,
	 const std::string& province_source)
{
	bool added_any = false;
	for (const auto& candidate_key: buildKeyVariants(title_key))
	{
		if (framework.locations.contains(candidate_key))
		{
			added_any = addLocation(accumulator, candidate_key, location_source) || added_any;
		}

		const auto province_locations = framework.locationsForProvince(candidate_key + "_province");
		if (!province_locations.empty())
		{
			added_any = addLocations(accumulator, province_locations, province_source) || added_any;
		}
	}
	return added_any;
}

bool addMatchForDisplayName(CandidateAccumulator& accumulator,
	 const std::string& display_name,
	 const Eu5LookupIndex& index,
	 const std::string& source)
{
	const auto normalized = normalizeName(display_name);
	if (normalized.empty())
	{
		return false;
	}

	const auto it = index.unique_display_names.find(normalized);
	if (it == index.unique_display_names.end() || it->second.empty())
	{
		return false;
	}

	return addLocation(accumulator, it->second, source);
}

std::map<std::string, std::vector<std::string>> loadExistingMappings(const std::filesystem::path& path)
{
	std::map<std::string, std::vector<std::string>> mappings;
	for (const auto& row: common::CsvReader::readFile(path))
	{
		const auto county_key = row.get("ck3_county");
		if (county_key.empty())
		{
			continue;
		}
		auto locations = common::split(row.get("eu5_locations"), '|');
		locations.erase(std::remove_if(locations.begin(), locations.end(), [](const std::string& location) {
			return location.empty();
		}), locations.end());
		mappings[county_key] = std::move(locations);
	}
	return mappings;
}

}  // namespace

BootstrapProvinceMappingResult BootstrapProvinceMappingGenerator::generate(const ck3::InstalledTitles& installed_titles,
	 const eu5::WorldFramework& framework,
	 const std::filesystem::path& existing_province_mappings_path,
	 diagnostics::DiagnosticsReport& diagnostics) const
{
	BootstrapProvinceMappingResult result;
	result.total_counties = installed_titles.counties.size();

	const auto existing_mappings = loadExistingMappings(existing_province_mappings_path);
	const auto index = buildIndex(framework);

	for (const auto& [county_key, county]: installed_titles.counties)
	{
		BootstrapProvinceMapping mapping;
		mapping.ck3_county = county_key;
		mapping.county_display_name = county.display_name;
		mapping.duchy_key = county.duchy_key;
		mapping.kingdom_key = county.kingdom_key;
		mapping.empire_key = county.empire_key;
		if (!county.baronies.empty())
		{
			mapping.primary_barony_key = county.baronies.front().key;
			mapping.primary_barony_display_name = county.baronies.front().display_name;
		}

		const auto existing_it = existing_mappings.find(county_key);
		if (existing_it != existing_mappings.end())
		{
			mapping.eu5_locations = existing_it->second;
			mapping.sources = {"existing_manual"};
			++result.manual_counties;
			++result.source_counts["existing_manual"];
			result.mappings.push_back(std::move(mapping));
			continue;
		}

		CandidateAccumulator accumulator;
		addMatchesForKey(accumulator,
			 county.key,
			 framework,
			 "county_exact_location_key",
			 "county_exact_province_definition");
		addMatchForDisplayName(accumulator, county.display_name, index, "county_exact_display_name");

		for (const auto& barony: county.baronies)
		{
			addMatchesForKey(accumulator,
				 barony.key,
				 framework,
				 "barony_exact_location_key",
				 "barony_exact_province_definition");
			addMatchForDisplayName(accumulator, barony.display_name, index, "barony_exact_display_name");
		}

		if (accumulator.locations.empty())
		{
			++result.unmapped_counties;
		}
		else
		{
			mapping.eu5_locations = std::move(accumulator.locations);
			mapping.sources = std::move(accumulator.sources);
			++result.generated_counties;
			for (const auto& source: mapping.sources)
			{
				++result.source_counts[source];
			}
		}

		result.mappings.push_back(std::move(mapping));
	}

	diagnostics.info("BOOTSTRAP_PROVINCE_MAPPING_COUNTS",
		 "Generated province bootstrap coverage for " + std::to_string(result.total_counties) + " CK3 counties: " +
				 std::to_string(result.manual_counties) + " manual, " + std::to_string(result.generated_counties) + " generated, " +
				 std::to_string(result.unmapped_counties) + " unmapped.");

	return result;
}

void BootstrapProvinceMappingGenerator::writeCsvs(const BootstrapProvinceMappingResult& result,
	 const std::filesystem::path& province_mappings_path,
	 const std::filesystem::path& report_path) const
{
	std::ostringstream mappings_csv;
	mappings_csv << "ck3_county,eu5_locations\n";
	for (const auto& mapping: result.mappings)
	{
		if (mapping.eu5_locations.empty())
		{
			continue;
		}
		mappings_csv << csvEscape(mapping.ck3_county) << ',' << csvEscape(common::join(mapping.eu5_locations, "|")) << '\n';
	}
	common::writeTextFile(province_mappings_path, mappings_csv.str(), common::TextEncoding::Utf8NoBom);

	std::ostringstream report_csv;
	report_csv << "ck3_county,county_display_name,duchy_key,kingdom_key,empire_key,primary_barony_key,primary_barony_display_name,eu5_locations,match_sources\n";
	for (const auto& mapping: result.mappings)
	{
		report_csv << csvEscape(mapping.ck3_county) << ',';
		report_csv << csvEscape(mapping.county_display_name) << ',';
		report_csv << csvEscape(mapping.duchy_key) << ',';
		report_csv << csvEscape(mapping.kingdom_key) << ',';
		report_csv << csvEscape(mapping.empire_key) << ',';
		report_csv << csvEscape(mapping.primary_barony_key) << ',';
		report_csv << csvEscape(mapping.primary_barony_display_name) << ',';
		report_csv << csvEscape(common::join(mapping.eu5_locations, "|")) << ',';
		report_csv << csvEscape(common::join(mapping.sources, "|")) << '\n';
	}
	common::writeTextFile(report_path, report_csv.str(), common::TextEncoding::Utf8NoBom);
}

}  // namespace ck3eu5::mappers
