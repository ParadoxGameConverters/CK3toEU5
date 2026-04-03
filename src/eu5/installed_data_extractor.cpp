#include "eu5/installed_data_extractor.h"

#include "common/filesystem_utils.h"
#include "common/pds_node.h"
#include "common/pds_parser.h"
#include "common/string_utils.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace ck3eu5::eu5 {
namespace fs = std::filesystem;

namespace {

std::string stripUtf8Bom(std::string text)
{
	if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF && static_cast<unsigned char>(text[1]) == 0xBB &&
		 static_cast<unsigned char>(text[2]) == 0xBF)
	{
		text.erase(0, 3);
	}
	return text;
}

std::string readGameTextFile(const fs::path& path)
{
	return stripUtf8Bom(common::readTextFile(path));
}

std::vector<fs::path> listFiles(const fs::path& directory, const std::string_view extension = {})
{
	std::vector<fs::path> paths;
	if (!fs::exists(directory))
	{
		return paths;
	}

	for (const auto& entry: fs::directory_iterator(directory))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}
		if (!extension.empty() && common::toLower(entry.path().extension().string()) != common::toLower(extension))
		{
			continue;
		}
		paths.push_back(entry.path());
	}

	std::sort(paths.begin(), paths.end());
	return paths;
}

common::PdsNode parseFile(const fs::path& path)
{
	common::PdsParser parser;
	return parser.parse(readGameTextFile(path));
}

std::string humanizeKey(std::string key)
{
	for (char& character: key)
	{
		if (character == '_')
		{
			character = ' ';
		}
	}

	bool capitalize_next = true;
	for (char& character: key)
	{
		if (std::isspace(static_cast<unsigned char>(character)))
		{
			capitalize_next = true;
			continue;
		}
		if (capitalize_next)
		{
			character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
			capitalize_next = false;
		}
		else
		{
			character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
		}
	}

	return key;
}

struct AssignmentView
{
	std::string key;
	std::string value;
	size_t next_position = 0;
};

bool isTokenBoundary(const char character)
{
	return character == '\0' || std::isspace(static_cast<unsigned char>(character)) || character == '=' || character == '{' ||
			 character == '}' || character == '#';
}

size_t skipWhitespaceAndComments(const std::string_view text, size_t position)
{
	while (position < text.size())
	{
		const char current = text[position];
		if (std::isspace(static_cast<unsigned char>(current)))
		{
			++position;
			continue;
		}
		if (current == '#')
		{
			while (position < text.size() && text[position] != '\n')
			{
				++position;
			}
			continue;
		}
		break;
	}
	return position;
}

size_t parseQuotedTokenEnd(const std::string_view text, const size_t position)
{
	size_t cursor = position + 1;
	while (cursor < text.size())
	{
		if (text[cursor] == '\\')
		{
			cursor += 2;
			continue;
		}
		if (text[cursor] == '"')
		{
			return cursor + 1;
		}
		++cursor;
	}
	throw std::runtime_error("Unterminated quoted token while scanning EU5 data.");
}

size_t parseBareTokenEnd(const std::string_view text, size_t position)
{
	while (position < text.size() && !isTokenBoundary(text[position]))
	{
		++position;
	}
	return position;
}

std::string stripToken(const std::string_view token)
{
	return common::stripQuotes(common::trim(token));
}

size_t findMatchingBrace(const std::string_view text, const size_t open_brace_position)
{
	if (open_brace_position >= text.size() || text[open_brace_position] != '{')
	{
		throw std::runtime_error("Internal error: expected opening brace while scanning EU5 data.");
	}

	size_t depth = 0;
	bool in_string = false;
	for (size_t position = open_brace_position; position < text.size(); ++position)
	{
		const char current = text[position];
		if (in_string)
		{
			if (current == '\\')
			{
				++position;
				continue;
			}
			if (current == '"')
			{
				in_string = false;
			}
			continue;
		}

		if (current == '"')
		{
			in_string = true;
			continue;
		}
		if (current == '{')
		{
			++depth;
			continue;
		}
		if (current == '}')
		{
			--depth;
			if (depth == 0)
			{
				return position;
			}
		}
	}

	throw std::runtime_error("Unbalanced braces while scanning EU5 data.");
}

std::optional<AssignmentView> nextAssignment(const std::string_view text, size_t position, const bool block_mode)
{
	position = skipWhitespaceAndComments(text, position);
	if (block_mode)
	{
		if (position >= text.size() - 1 || text[position] == '}')
		{
			return std::nullopt;
		}
	}
	else if (position >= text.size())
	{
		return std::nullopt;
	}

	const size_t key_start = position;
	const size_t key_end = text[position] == '"' ? parseQuotedTokenEnd(text, position) : parseBareTokenEnd(text, position);
	const auto key = stripToken(text.substr(key_start, key_end - key_start));

	position = skipWhitespaceAndComments(text, key_end);
	if (position >= text.size() || text[position] != '=')
	{
		throw std::runtime_error("Malformed assignment while scanning EU5 data.");
	}

	position = skipWhitespaceAndComments(text, position + 1);
	if (position >= text.size())
	{
		throw std::runtime_error("Missing value while scanning EU5 data.");
	}

	const size_t value_start = position;
	size_t value_end = position;
	if (text[position] == '{')
	{
		value_end = findMatchingBrace(text, position) + 1;
	}
	else if (text[position] == '"')
	{
		value_end = parseQuotedTokenEnd(text, position);
	}
	else
	{
		value_end = parseBareTokenEnd(text, position);
		if (value_end < text.size())
		{
			value_end = skipWhitespaceAndComments(text, value_end);
			if (value_end < text.size() && text[value_end] == '{')
			{
				value_end = findMatchingBrace(text, value_end) + 1;
			}
		}
	}

	return AssignmentView{.key = key, .value = common::trim(text.substr(value_start, value_end - value_start)), .next_position = value_end};
}

std::optional<std::string> findRootAssignment(const std::string_view text, const std::string_view key)
{
	size_t position = 0;
	while (const auto assignment = nextAssignment(text, position, false))
	{
		if (assignment->key == key)
		{
			return assignment->value;
		}
		position = assignment->next_position;
	}
	return std::nullopt;
}

std::vector<AssignmentView> parseAssignmentsInBlock(const std::string_view block_text)
{
	if (block_text.empty() || block_text.front() != '{' || block_text.back() != '}')
	{
		throw std::runtime_error("Internal error: expected block while scanning EU5 data.");
	}

	std::vector<AssignmentView> assignments;
	size_t position = 1;
	while (const auto assignment = nextAssignment(block_text, position, true))
	{
		assignments.push_back(*assignment);
		position = assignment->next_position;
	}
	return assignments;
}

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

void setIfEmpty(std::string& target, const std::string& value)
{
	if (target.empty())
	{
		target = value;
	}
}

void ensureLocationKey(WorldFramework& framework, const std::string& location_key)
{
	auto& location = framework.locations[location_key];
	if (location.key.empty())
	{
		location.key = location_key;
	}
	if (location.display_name.empty())
	{
		location.display_name = humanizeKey(location_key);
	}
}

bool scalarItemsOnly(const common::PdsNode& node)
{
	return !node.items().empty() && node.properties().empty() &&
			 std::all_of(node.items().begin(), node.items().end(), [](const common::PdsNode& item) { return item.isScalar(); });
}

void assignProvinceDefinition(WorldFramework& framework,
	 const std::string& location_key,
	 const std::string& province_definition,
	 const std::string& region,
	 const std::string& area)
{
	ensureLocationKey(framework, location_key);
	auto& location = framework.locations[location_key];

	if (!province_definition.empty())
	{
		if (!location.province_definition.empty() && location.province_definition != province_definition)
		{
			const auto existing_it = framework.province_to_locations.find(location.province_definition);
			if (existing_it != framework.province_to_locations.end())
			{
				auto& existing_locations = existing_it->second;
				existing_locations.erase(std::remove(existing_locations.begin(), existing_locations.end(), location_key), existing_locations.end());
			}
		}

		location.province_definition = province_definition;
		auto& province_locations = framework.province_to_locations[province_definition];
		if (std::find(province_locations.begin(), province_locations.end(), location_key) == province_locations.end())
		{
			province_locations.push_back(location_key);
		}
	}

	if (!region.empty())
	{
		location.region = region;
	}
	if (!area.empty())
	{
		location.area = area;
	}
}

void walkLocationHierarchy(const common::PdsNode& node,
	 const std::string& current_region,
	 const std::string& current_area,
	 WorldFramework& framework)
{
	for (const auto& [key, value]: node.properties())
	{
		if (scalarItemsOnly(value))
		{
			for (const auto& item: value.items())
			{
				assignProvinceDefinition(framework, item.scalar(), key, current_region, current_area);
			}
			continue;
		}

		if (key.ends_with("_region"))
		{
			walkLocationHierarchy(value, key, current_area, framework);
			continue;
		}
		if (key.ends_with("_area"))
		{
			walkLocationHierarchy(value, current_region, key, framework);
			continue;
		}

		walkLocationHierarchy(value, current_region, current_area, framework);
	}
}

std::set<std::string> loadCoastalPorts(const fs::path& ports_path)
{
	std::set<std::string> coastal_locations;
	if (!fs::exists(ports_path))
	{
		return coastal_locations;
	}

	std::istringstream input(readGameTextFile(ports_path));
	std::string line;
	bool first = true;
	while (std::getline(input, line))
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		if (first)
		{
			first = false;
			continue;
		}
		const auto trimmed = common::trim(line);
		if (trimmed.empty())
		{
			continue;
		}

		const auto fields = common::split(trimmed, ';', false);
		if (!fields.empty() && !fields.front().empty())
		{
			coastal_locations.insert(fields.front());
		}
	}

	return coastal_locations;
}

std::map<std::string, std::string> loadNamedColors(const fs::path& named_colors_directory)
{
	std::map<std::string, std::string> colors;
	for (const auto& path: listFiles(named_colors_directory, ".txt"))
	{
		const auto color_block = findRootAssignment(readGameTextFile(path), "colors");
		if (!color_block.has_value())
		{
			continue;
		}

		for (const auto& assignment: parseAssignmentsInBlock(*color_block))
		{
			colors[assignment.key] = assignment.value;
		}
	}
	return colors;
}

std::string resolveColorValue(std::string value, const std::map<std::string, std::string>& named_colors)
{
	for (int depth = 0; depth < 8; ++depth)
	{
		const auto it = named_colors.find(value);
		if (it == named_colors.end())
		{
			break;
		}
		if (it->second == value)
		{
			break;
		}
		value = it->second;
	}
	return value;
}

bool looksLikeTag(const std::string& key)
{
	if (key.size() != 3)
	{
		return false;
	}
	return std::all_of(key.begin(), key.end(), [](const char character) {
		return std::isupper(static_cast<unsigned char>(character)) != 0;
	});
}

void loadNamedLocations(const fs::path& named_locations_directory, WorldFramework& framework)
{
	for (const auto& path: listFiles(named_locations_directory, ".txt"))
	{
		const auto root = parseFile(path);
		for (const auto& [key, value]: root.properties())
		{
			if (!value.isScalar())
			{
				continue;
			}
			ensureLocationKey(framework, key);
		}
	}
}

void loadLocationTemplates(const fs::path& location_templates_path,
	 const std::set<std::string>& coastal_ports,
	 WorldFramework& framework)
{
	const auto root = parseFile(location_templates_path);

	for (const auto& [key, value]: root.properties())
	{
		if (!framework.locations.contains(key))
		{
			continue;
		}
		auto& location = framework.locations[key];
		ensureLocationKey(framework, key);
		location.topography = value.getString("topography", location.topography);
		location.climate = value.getString("climate", location.climate);
		location.raw_good = value.getString("raw_material", location.raw_good.empty() ? "grain" : location.raw_good);
		location.has_port = location.has_port || coastal_ports.contains(key) || value.getDouble("natural_harbor_suitability", 0.0) > 0.0;
		location.coastal = location.coastal || location.has_port;
	}
}

void loadCityRanks(const fs::path& city_ranks_path, WorldFramework& framework)
{
	const auto root = parseFile(city_ranks_path);
	const auto* locations = root.get("locations");
	if (!locations)
	{
		return;
	}

	for (const auto& [key, value]: locations->properties())
	{
		if (!framework.locations.contains(key))
		{
			continue;
		}
		auto& location = framework.locations[key];
		ensureLocationKey(framework, key);
		location.default_rank = value.getString("rank", location.default_rank.empty() ? "rural_settlement" : location.default_rank);
		location.town_setup = value.getString("town_setup", location.town_setup);
	}
}

void loadCountryColors(const fs::path& countries_directory,
	 const std::map<std::string, std::string>& named_colors,
	 WorldFramework& framework)
{
	for (const auto& path: listFiles(countries_directory, ".txt"))
	{
		for (const auto& assignment: parseAssignmentsInBlock("{" + readGameTextFile(path) + "}"))
		{
			const auto& key = assignment.key;
			if (!looksLikeTag(key))
			{
				continue;
			}
			if (assignment.value.empty() || assignment.value.front() != '{' || assignment.value.back() != '}')
			{
				continue;
			}

			auto& definition = framework.colors[key];
			definition.tag = key;
			for (const auto& field: parseAssignmentsInBlock(assignment.value))
			{
				if (field.key == "color")
				{
					definition.color = resolveColorValue(field.value, named_colors);
				}
				else if (field.key == "color2")
				{
					definition.color2 = resolveColorValue(field.value, named_colors);
				}
				else if (field.key == "color3")
				{
					definition.color3 = resolveColorValue(field.value, named_colors);
				}
				else if (field.key == "unit_color0")
				{
					definition.unit_color0 = resolveColorValue(field.value, named_colors);
				}
				else if (field.key == "unit_color1")
				{
					definition.unit_color1 = resolveColorValue(field.value, named_colors);
				}
				else if (field.key == "unit_color2")
				{
					definition.unit_color2 = resolveColorValue(field.value, named_colors);
				}
				else if (field.key == "description_category")
				{
					definition.description_category = stripToken(field.value);
				}
				else if (field.key == "difficulty")
				{
					definition.difficulty = common::parseInt(field.value).value_or(definition.difficulty);
				}
			}

			setIfEmpty(definition.color2, definition.color);
			setIfEmpty(definition.color3, definition.color);
			setIfEmpty(definition.unit_color0, definition.color);
			setIfEmpty(definition.unit_color1, definition.color2);
			setIfEmpty(definition.unit_color2, definition.color3);
		}
	}
}

}  // namespace

WorldFramework InstalledDataExtractor::extract(const fs::path& game_path, diagnostics::DiagnosticsReport& diagnostics) const
{
	if (!fs::exists(game_path))
	{
		throw std::runtime_error("EU5 game path does not exist: " + game_path.string());
	}

	const fs::path named_locations_directory = game_path / "in_game/map_data/named_locations";
	const fs::path definitions_path = game_path / "in_game/map_data/definitions.txt";
	const fs::path location_templates_path = game_path / "in_game/map_data/location_templates.txt";
	const fs::path ports_path = game_path / "in_game/map_data/ports.csv";
	const fs::path city_ranks_path = game_path / "main_menu/setup/start/07_cities_and_buildings.txt";
	const fs::path named_colors_directory = game_path / "main_menu/common/named_colors";
	const fs::path countries_directory = game_path / "in_game/setup/countries";

	for (const auto& required_path: {named_locations_directory, definitions_path, location_templates_path, city_ranks_path, named_colors_directory, countries_directory})
	{
		if (!fs::exists(required_path))
		{
			throw std::runtime_error("Required EU5 data path is missing: " + required_path.string());
		}
	}

	WorldFramework framework;

	loadNamedLocations(named_locations_directory, framework);
	const auto coastal_ports = loadCoastalPorts(ports_path);
	walkLocationHierarchy(parseFile(definitions_path), {}, {}, framework);
	loadLocationTemplates(location_templates_path, coastal_ports, framework);
	loadCityRanks(city_ranks_path, framework);
	const auto named_colors = loadNamedColors(named_colors_directory);
	loadCountryColors(countries_directory, named_colors, framework);

	for (auto& [key, location]: framework.locations)
	{
		ensureLocationKey(framework, key);
		if (location.raw_good.empty())
		{
			location.raw_good = "grain";
		}
		if (location.default_rank.empty())
		{
			location.default_rank = "rural_settlement";
		}
	}

	if (framework.locations.empty())
	{
		diagnostics.error("EU5_EXTRACT_NO_LOCATIONS", "No EU5 locations were extracted from the installed game.");
	}
	if (framework.colors.empty())
	{
		diagnostics.warning("EU5_EXTRACT_NO_COLORS", "No country colors were extracted from the installed game.");
	}

	diagnostics.info("EU5_EXTRACT_COUNTS",
		 "Extracted " + std::to_string(framework.locations.size()) + " locations and " + std::to_string(framework.colors.size()) +
				 " country color definitions from " + game_path.string() + '.');

	return framework;
}

void InstalledDataExtractor::writeCsvs(const WorldFramework& framework,
	 const fs::path& location_framework_path,
	 const fs::path& country_colors_path) const
{
	std::ostringstream location_csv;
	location_csv << "location_key,province_definition,display_name,raw_good,region,area,climate,topography,default_rank,town_setup,coastal,has_port\n";
	for (const auto& [key, location]: framework.locations)
	{
		location_csv << csvEscape(key) << ',';
		location_csv << csvEscape(location.province_definition) << ',';
		location_csv << csvEscape(location.display_name) << ',';
		location_csv << csvEscape(location.raw_good) << ',';
		location_csv << csvEscape(location.region) << ',';
		location_csv << csvEscape(location.area) << ',';
		location_csv << csvEscape(location.climate) << ',';
		location_csv << csvEscape(location.topography) << ',';
		location_csv << csvEscape(location.default_rank) << ',';
		location_csv << csvEscape(location.town_setup) << ',';
		location_csv << (location.coastal ? "yes" : "no") << ',';
		location_csv << (location.has_port ? "yes" : "no") << '\n';
	}
	common::writeTextFile(location_framework_path, location_csv.str(), common::TextEncoding::Utf8NoBom);

	std::ostringstream color_csv;
	color_csv << "tag,color,color2,color3,unit_color0,unit_color1,unit_color2,description_category,difficulty\n";
	for (const auto& [tag, color]: framework.colors)
	{
		color_csv << csvEscape(tag) << ',';
		color_csv << csvEscape(color.color) << ',';
		color_csv << csvEscape(color.color2) << ',';
		color_csv << csvEscape(color.color3) << ',';
		color_csv << csvEscape(color.unit_color0) << ',';
		color_csv << csvEscape(color.unit_color1) << ',';
		color_csv << csvEscape(color.unit_color2) << ',';
		color_csv << csvEscape(color.description_category) << ',';
		color_csv << color.difficulty << '\n';
	}
	common::writeTextFile(country_colors_path, color_csv.str(), common::TextEncoding::Utf8NoBom);
}

}  // namespace ck3eu5::eu5
