#include "eu5/installed_definitions_loader.h"

#include "common/filesystem_utils.h"
#include "common/pds_parser.h"
#include "common/string_utils.h"

#include <algorithm>
#include <filesystem>
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

std::vector<fs::path> listTxtFiles(const fs::path& directory)
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
		if (common::toLower(entry.path().extension().string()) != ".txt")
		{
			continue;
		}
		if (common::toLower(entry.path().extension().string()) == ".info")
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
	return parser.parse(stripUtf8Bom(common::readTextFile(path)));
}

void loadTopLevelKeysIntoSet(const fs::path& directory, std::set<std::string>& target)
{
	for (const auto& path: listTxtFiles(directory))
	{
		const auto root = parseFile(path);
		for (const auto& [key, value]: root.properties())
		{
			if (!value.isScalar())
			{
				target.insert(key);
			}
		}
	}
}

void loadLawValuesIntoSet(const fs::path& directory, std::set<std::string>& target)
{
	for (const auto& path: listTxtFiles(directory))
	{
		const auto root = parseFile(path);
		for (const auto& [category_key, category_value]: root.properties())
		{
			if (category_value.isScalar())
			{
				continue;
			}
			for (const auto& [law_value, law_node]: category_value.properties())
			{
				if (!law_node.isScalar())
				{
					target.insert(law_value);
				}
			}
		}
	}
}

}  // namespace

InstalledDefinitions InstalledDefinitionsLoader::load(const fs::path& game_path,
	 diagnostics::DiagnosticsReport& diagnostics) const
{
	if (!fs::exists(game_path))
	{
		throw std::runtime_error("EU5 game path does not exist: " + game_path.string());
	}

	const fs::path common_root = game_path / "in_game/common";
	const fs::path cultures_directory = common_root / "cultures";
	const fs::path religions_directory = common_root / "religions";
	const fs::path government_types_directory = common_root / "government_types";
	const fs::path country_ranks_directory = common_root / "country_ranks";
	const fs::path pop_types_directory = common_root / "pop_types";
	const fs::path heir_selections_directory = common_root / "heir_selections";
	const fs::path laws_directory = common_root / "laws";
	const fs::path government_reforms_directory = common_root / "government_reforms";
	const fs::path estate_privileges_directory = common_root / "estate_privileges";
	const fs::path subject_types_directory = common_root / "subject_types";
	const fs::path building_types_directory = common_root / "building_types";
	const fs::path unit_types_directory = common_root / "unit_types";
	const fs::path scripted_relations_directory = common_root / "scripted_relations";
	const fs::path subject_military_stances_directory = common_root / "subject_military_stances";

	for (const auto& required_path:
		 {cultures_directory,
			 religions_directory,
			 government_types_directory,
			 country_ranks_directory,
			 pop_types_directory,
			 heir_selections_directory,
			 laws_directory,
			 government_reforms_directory,
			 estate_privileges_directory,
			 subject_types_directory,
			 building_types_directory,
			 unit_types_directory,
			 scripted_relations_directory,
			 subject_military_stances_directory})
	{
		if (!fs::exists(required_path))
		{
			throw std::runtime_error("Required EU5 definitions path is missing: " + required_path.string());
		}
	}

	InstalledDefinitions definitions;
	loadTopLevelKeysIntoSet(cultures_directory, definitions.cultures);
	loadTopLevelKeysIntoSet(religions_directory, definitions.religions);
	loadTopLevelKeysIntoSet(government_types_directory, definitions.government_types);
	loadTopLevelKeysIntoSet(country_ranks_directory, definitions.country_ranks);
	loadTopLevelKeysIntoSet(pop_types_directory, definitions.pop_types);
	loadTopLevelKeysIntoSet(heir_selections_directory, definitions.heir_selections);
	loadLawValuesIntoSet(laws_directory, definitions.law_values);
	loadTopLevelKeysIntoSet(government_reforms_directory, definitions.government_reforms);
	loadTopLevelKeysIntoSet(estate_privileges_directory, definitions.estate_privileges);
	loadTopLevelKeysIntoSet(subject_types_directory, definitions.subject_types);
	loadTopLevelKeysIntoSet(building_types_directory, definitions.building_types);
	loadTopLevelKeysIntoSet(unit_types_directory, definitions.unit_types);
	loadTopLevelKeysIntoSet(scripted_relations_directory, definitions.scripted_relations);
	loadTopLevelKeysIntoSet(subject_military_stances_directory, definitions.subject_military_stances);

	diagnostics.info("EU5_DEFINITIONS_COUNTS",
		 "Loaded " + std::to_string(definitions.cultures.size()) + " cultures, " +
				 std::to_string(definitions.religions.size()) + " religions, " +
				 std::to_string(definitions.government_types.size()) + " government types, " +
				 std::to_string(definitions.country_ranks.size()) + " country ranks, " +
				 std::to_string(definitions.pop_types.size()) + " pop types, " +
				 std::to_string(definitions.heir_selections.size()) + " heir selections, " +
				 std::to_string(definitions.law_values.size()) + " law values, " +
				 std::to_string(definitions.government_reforms.size()) + " government reforms, " +
				 std::to_string(definitions.estate_privileges.size()) + " estate privileges, and " +
				 std::to_string(definitions.subject_types.size()) + " subject types, " +
				 std::to_string(definitions.building_types.size()) + " building types, " +
				 std::to_string(definitions.unit_types.size()) + " unit types, " +
				 std::to_string(definitions.scripted_relations.size()) + " scripted relations, and " +
				 std::to_string(definitions.subject_military_stances.size()) + " subject military stances from " + game_path.string() +
				 '.');

	return definitions;
}

}  // namespace ck3eu5::eu5
