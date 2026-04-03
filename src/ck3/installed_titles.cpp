#include "ck3/installed_titles.h"

#include "ck3/world.h"
#include "common/filesystem_utils.h"
#include "common/pds_node.h"
#include "common/pds_parser.h"
#include "common/string_utils.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace ck3eu5::ck3 {
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

std::vector<fs::path> listFiles(const fs::path& directory, const std::string_view extension)
{
	std::vector<fs::path> paths;
	if (!fs::exists(directory))
	{
		return paths;
	}

	for (const auto& entry: fs::recursive_directory_iterator(directory))
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

std::string extractQuotedValue(const std::string& line)
{
	const auto first_quote = line.find('"');
	if (first_quote == std::string::npos)
	{
		return {};
	}
	const auto second_quote = line.find('"', first_quote + 1);
	if (second_quote == std::string::npos || second_quote <= first_quote + 1)
	{
		return {};
	}
	return line.substr(first_quote + 1, second_quote - first_quote - 1);
}

common::PdsNode parseFile(const fs::path& path)
{
	common::PdsParser parser;
	return parser.parse(readGameTextFile(path));
}

std::string humanizeTitleKey(const std::string& key)
{
	std::string name = key;
	if (name.size() > 2 && name[1] == '_')
	{
		name.erase(0, 2);
	}

	for (char& character: name)
	{
		if (character == '_')
		{
			character = ' ';
		}
	}

	bool capitalize_next = true;
	for (char& character: name)
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

	return name;
}

void appendUnique(std::vector<std::string>& values, const std::string& value)
{
	if (!value.empty() && std::find(values.begin(), values.end(), value) == values.end())
	{
		values.push_back(value);
	}
}

struct TitleContext
{
	std::string empire_key;
	std::string kingdom_key;
	std::string duchy_key;
	std::string county_key;
};

std::optional<std::pair<std::string, std::string>> parseLocalizationLine(std::string_view line)
{
	if (line.empty())
	{
		return std::nullopt;
	}

	if (!line.empty() && line.back() == '\r')
	{
		line.remove_suffix(1);
	}

	auto trimmed = common::trim(line);
	if (trimmed.empty() || trimmed.starts_with('#') || trimmed == "l_english:")
	{
		return std::nullopt;
	}

	const auto colon_position = trimmed.find(':');
	if (colon_position == std::string::npos || colon_position == 0)
	{
		return std::nullopt;
	}

	const auto key = common::trim(trimmed.substr(0, colon_position));
	size_t position = colon_position + 1;
	while (position < trimmed.size() && std::isdigit(static_cast<unsigned char>(trimmed[position])))
	{
		++position;
	}
	while (position < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[position])))
	{
		++position;
	}
	if (position >= trimmed.size() || trimmed[position] != '"')
	{
		return std::nullopt;
	}

	++position;
	std::string value;
	bool escaping = false;
	for (; position < trimmed.size(); ++position)
	{
		const char character = trimmed[position];
		if (escaping)
		{
			value.push_back(character);
			escaping = false;
			continue;
		}
		if (character == '\\')
		{
			escaping = true;
			continue;
		}
		if (character == '"')
		{
			return std::pair<std::string, std::string>{key, value};
		}
		value.push_back(character);
	}

	return std::nullopt;
}

void ensureInstalledTitle(const std::string& key,
	 const TitleRank rank,
	 const std::string& de_jure_liege_title,
	 InstalledTitles& titles)
{
	auto& title = titles.titles[key];
	if (title.key.empty())
	{
		title.key = key;
		title.rank = rank;
		title.display_name = titles.localizedNameFor(key);
	}
	if (title.rank == TitleRank::Unknown)
	{
		title.rank = rank;
	}
	if (title.display_name.empty())
	{
		title.display_name = titles.localizedNameFor(key);
	}
	if (title.de_jure_liege_title.empty())
	{
		title.de_jure_liege_title = de_jure_liege_title;
	}
	if (!de_jure_liege_title.empty())
	{
		auto& liege = titles.titles[de_jure_liege_title];
		if (liege.key.empty())
		{
			liege.key = de_jure_liege_title;
			liege.rank = titleRankFromTitleKey(de_jure_liege_title);
			liege.display_name = titles.localizedNameFor(de_jure_liege_title);
		}
		appendUnique(liege.de_jure_vassals, key);
	}
}

void loadLocalizations(const fs::path& localization_directory, InstalledTitles& titles)
{
	for (const auto& path: listFiles(localization_directory, ".yml"))
	{
		std::istringstream input(readGameTextFile(path));
		std::string line;
		while (std::getline(input, line))
		{
			const auto parsed = parseLocalizationLine(line);
			if (!parsed.has_value())
			{
				continue;
			}
			titles.localizations[parsed->first] = parsed->second;
		}
	}
}

fs::path resolveModRoot(const fs::path& mod_path)
{
	if (fs::is_directory(mod_path))
	{
		return mod_path;
	}

	if (!fs::exists(mod_path))
	{
		throw std::runtime_error("CK3 mod path does not exist: " + mod_path.string());
	}

	const auto descriptor_text = readGameTextFile(mod_path);
	std::istringstream input(descriptor_text);
	std::string line;
	while (std::getline(input, line))
	{
		const auto trimmed = common::trim(line);
		if (!trimmed.starts_with("path="))
		{
			continue;
		}
		const auto descriptor_value = extractQuotedValue(trimmed);
		if (descriptor_value.empty())
		{
			break;
		}
		fs::path resolved_path(descriptor_value);
		if (resolved_path.is_relative())
		{
			resolved_path = mod_path.parent_path() / resolved_path;
		}
		return resolved_path;
	}

	throw std::runtime_error("Could not resolve CK3 mod root from descriptor: " + mod_path.string());
}

void ensureCounty(const TitleContext& context, const std::string& county_key, InstalledTitles& titles)
{
	ensureInstalledTitle(county_key, TitleRank::County, context.duchy_key, titles);
	auto& county = titles.counties[county_key];
	if (county.key.empty())
	{
		county.key = county_key;
		county.display_name = titles.localizedNameFor(county_key);
		county.duchy_key = context.duchy_key;
		county.kingdom_key = context.kingdom_key;
		county.empire_key = context.empire_key;
	}
}

void walkTitles(const common::PdsNode& node, const TitleContext& context, InstalledTitles& titles)
{
	for (const auto& [key, value]: node.properties())
	{
		const auto rank = titleRankFromTitleKey(key);
		if (rank == TitleRank::Unknown)
		{
			if (!value.isScalar())
			{
				walkTitles(value, context, titles);
			}
			continue;
		}

		auto next_context = context;
		std::string de_jure_liege_title;
		switch (rank)
		{
			case TitleRank::Empire:
				de_jure_liege_title.clear();
				ensureInstalledTitle(key, rank, de_jure_liege_title, titles);
				next_context.empire_key = key;
				next_context.kingdom_key.clear();
				next_context.duchy_key.clear();
				next_context.county_key.clear();
				break;
			case TitleRank::Kingdom:
				de_jure_liege_title = context.empire_key;
				ensureInstalledTitle(key, rank, de_jure_liege_title, titles);
				next_context.kingdom_key = key;
				next_context.duchy_key.clear();
				next_context.county_key.clear();
				break;
			case TitleRank::Duchy:
				de_jure_liege_title = context.kingdom_key;
				ensureInstalledTitle(key, rank, de_jure_liege_title, titles);
				next_context.duchy_key = key;
				next_context.county_key.clear();
				break;
			case TitleRank::County:
				de_jure_liege_title = context.duchy_key;
				ensureInstalledTitle(key, rank, de_jure_liege_title, titles);
				next_context.county_key = key;
				ensureCounty(next_context, key, titles);
				break;
			case TitleRank::Barony:
				if (!context.county_key.empty())
				{
					ensureInstalledTitle(key, rank, context.county_key, titles);
					ensureCounty(context, context.county_key, titles);
					auto& county = titles.counties[context.county_key];
					const auto province_id = value.getInt("province", 0);
					auto& title = titles.titles[key];
					if (title.province_id <= 0 && province_id > 0)
					{
						title.province_id = province_id;
					}
					const auto duplicate = std::find_if(county.baronies.begin(), county.baronies.end(), [&](const InstalledBaronyTitle& barony) {
						return barony.key == key;
					});
					if (duplicate == county.baronies.end())
					{
						county.baronies.push_back({.key = key, .display_name = titles.localizedNameFor(key), .province_id = province_id});
					}
					else if (duplicate->province_id <= 0 && province_id > 0)
					{
						duplicate->province_id = province_id;
					}
				}
				break;
			case TitleRank::Unknown:
				break;
		}

		if (!value.isScalar())
		{
			walkTitles(value, next_context, titles);
		}
	}
}

void loadCultures(const fs::path& cultures_directory, InstalledTitles& titles)
{
	if (!fs::exists(cultures_directory))
	{
		return;
	}

	for (const auto& path: listFiles(cultures_directory, ".txt"))
	{
		const auto root = parseFile(path);
		for (const auto& [key, value]: root.properties())
		{
			if (value.isScalar() || key.empty() || key.starts_with('_'))
			{
				continue;
			}

			auto& culture = titles.cultures[key];
			culture.key = key;
			if (culture.display_name.empty())
			{
				culture.display_name = titles.localizedNameFor(key);
			}
			if (culture.ethos.empty())
			{
				culture.ethos = value.getString("ethos");
			}
			if (culture.heritage.empty())
			{
				culture.heritage = value.getString("heritage");
			}
			if (culture.language.empty())
			{
				culture.language = value.getString("language");
			}
			for (const auto& parent: value.getListOfScalars("parents"))
			{
				appendUnique(culture.parents, parent);
			}
		}
	}
}

void loadReligions(const fs::path& religions_directory, InstalledTitles& titles)
{
	if (!fs::exists(religions_directory))
	{
		return;
	}

	for (const auto& path: listFiles(religions_directory, ".txt"))
	{
		const auto root = parseFile(path);
		for (const auto& [religion_key, religion_value]: root.properties())
		{
			if (religion_value.isScalar() || religion_key.empty() || religion_key.starts_with('_'))
			{
				continue;
			}

			auto& religion = titles.religions[religion_key];
			religion.key = religion_key;
			if (religion.display_name.empty())
			{
				religion.display_name = titles.localizedNameFor(religion_key);
			}
			if (religion.family.empty())
			{
				religion.family = religion_value.getString("family");
			}

			if (const auto* faiths = religion_value.get("faiths"))
			{
				for (const auto& [faith_key, faith_value]: faiths->properties())
				{
					if (faith_value.isScalar() || faith_key.empty() || faith_key.starts_with('_'))
					{
						continue;
					}

					auto& faith = titles.faiths[faith_key];
					faith.key = faith_key;
					if (faith.display_name.empty())
					{
						faith.display_name = titles.localizedNameFor(faith_key);
					}
					if (faith.religion_key.empty())
					{
						faith.religion_key = religion_key;
					}
					if (faith.religion_display_name.empty())
					{
						faith.religion_display_name = religion.display_name;
					}
					if (faith.religion_family.empty())
					{
						faith.religion_family = religion.family;
					}
					for (const auto& doctrine: faith_value.getAll("doctrine"))
					{
						appendUnique(faith.doctrines, doctrine->scalarOr());
					}
				}
			}
		}
	}
}

}  // namespace

std::string InstalledTitles::localizedNameFor(const std::string& title_key) const
{
	const auto localization = localizations.find(title_key);
	if (localization != localizations.end() && !localization->second.empty())
	{
		return localization->second;
	}
	return humanizeTitleKey(title_key);
}

std::string InstalledTitles::resolveLocalization(const std::string& value) const
{
	if (value.empty())
	{
		return {};
	}
	if (const auto localization = localizations.find(value); localization != localizations.end() && !localization->second.empty())
	{
		return localization->second;
	}
	return value;
}

InstalledTitles InstalledTitlesLoader::load(const fs::path& game_path, const std::vector<fs::path>& mod_paths) const
{
	if (!fs::exists(game_path))
	{
		throw std::runtime_error("CK3 game path does not exist: " + game_path.string());
	}

	const fs::path landed_titles_directory = game_path / "game/common/landed_titles";
	const fs::path cultures_directory = game_path / "game/common/culture/cultures";
	const fs::path religions_directory = game_path / "game/common/religion/religions";
	const fs::path localization_directory = game_path / "game/localization/english";

	for (const auto& required_path: {landed_titles_directory, localization_directory})
	{
		if (!fs::exists(required_path))
		{
			throw std::runtime_error("Required CK3 data path is missing: " + required_path.string());
		}
	}

	InstalledTitles titles;
	loadLocalizations(localization_directory, titles);

	for (const auto& path: listFiles(landed_titles_directory, ".txt"))
	{
		walkTitles(parseFile(path), {}, titles);
	}
	loadCultures(cultures_directory, titles);
	loadReligions(religions_directory, titles);

	for (const auto& mod_path: mod_paths)
	{
		const auto mod_root = resolveModRoot(mod_path);
		const auto mod_landed_titles = mod_root / "common/landed_titles";
		const auto mod_cultures = mod_root / "common/culture/cultures";
		const auto mod_religions = mod_root / "common/religion/religions";
		const auto mod_localization = mod_root / "localization";

		if (fs::exists(mod_localization))
		{
			loadLocalizations(mod_localization, titles);
		}
		if (!fs::exists(mod_landed_titles))
		{
			continue;
		}

		for (const auto& path: listFiles(mod_landed_titles, ".txt"))
		{
			walkTitles(parseFile(path), {}, titles);
		}
		loadCultures(mod_cultures, titles);
		loadReligions(mod_religions, titles);
	}

	return titles;
}

}  // namespace ck3eu5::ck3
