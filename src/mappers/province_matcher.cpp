#include "mappers/province_matcher.h"

#include "common/string_utils.h"

#include <set>

namespace ck3eu5::mappers {
namespace {

std::string stripCountyPrefix(std::string key)
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

	const auto stripped_key = stripCountyPrefix(title_key);
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

}  // namespace

ProvinceMatcher::ProvinceMatcher(const eu5::WorldFramework& framework): framework_(framework)
{
	for (const auto& [key, location]: framework_.locations)
	{
		const auto normalized = normalizeName(location.display_name);
		if (normalized.empty())
		{
			continue;
		}

		const auto [it, inserted] = unique_display_names_.emplace(normalized, key);
		if (!inserted && it->second != key)
		{
			it->second.clear();
		}
	}
}

std::optional<ProvinceMatch> ProvinceMatcher::match(const ck3::County& county) const
{
	std::vector<std::string> locations;
	std::vector<std::string> sources;
	std::set<std::string> seen_locations;
	std::set<std::string> seen_sources;

	const auto add_locations = [&](const std::vector<std::string>& values, const std::string& source) {
		bool added_any = false;
		for (const auto& value: values)
		{
			if (value.empty())
			{
				continue;
			}
			if (seen_locations.insert(value).second)
			{
				locations.push_back(value);
				added_any = true;
			}
		}
		if (added_any && seen_sources.insert(source).second)
		{
			sources.push_back(source);
		}
		return added_any;
	};

	const auto add_key_matches = [&](const std::string& title_key,
									  const std::string& location_source,
									  const std::string& province_source) {
		bool matched = false;
		for (const auto& candidate_key: buildKeyVariants(title_key))
		{
			if (framework_.locations.contains(candidate_key))
			{
				matched = add_locations({candidate_key}, location_source) || matched;
			}

			const auto province_locations = framework_.locationsForProvince(candidate_key + "_province");
			if (!province_locations.empty())
			{
				matched = add_locations(province_locations, province_source) || matched;
			}
		}
		return matched;
	};

	const auto add_display_name_match = [&](const std::string& display_name, const std::string& source) {
		const auto normalized = normalizeName(display_name);
		if (normalized.empty())
		{
			return false;
		}
		const auto display_it = unique_display_names_.find(normalized);
		if (display_it == unique_display_names_.end() || display_it->second.empty())
		{
			return false;
		}
		return add_locations({display_it->second}, source);
	};

	add_key_matches(county.key, "exact_location_key", "exact_province_definition");

	const auto display_name = normalizeName(county.display_name);
	if (!display_name.empty())
	{
		const auto display_it = unique_display_names_.find(display_name);
		if (display_it != unique_display_names_.end() && !display_it->second.empty())
		{
			add_locations({display_it->second}, "exact_display_name");
		}
	}

	for (const auto& barony_key: county.barony_keys)
	{
		add_key_matches(barony_key, "barony_exact_location_key", "barony_exact_province_definition");
	}

	for (const auto& barony_display_name: county.barony_display_names)
	{
		add_display_name_match(barony_display_name, "barony_exact_display_name");
	}

	if (!locations.empty())
	{
		return ProvinceMatch{.eu5_locations = std::move(locations), .source = common::join(sources, "|")};
	}

	return std::nullopt;
}

}  // namespace ck3eu5::mappers
