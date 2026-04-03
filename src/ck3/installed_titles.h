#pragma once

#include "ck3/world.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace ck3eu5::ck3 {

struct InstalledTitle
{
	std::string key;
	TitleRank rank = TitleRank::Unknown;
	std::string display_name;
	std::string de_jure_liege_title;
	std::vector<std::string> de_jure_vassals;
	int province_id = 0;
};

struct InstalledBaronyTitle
{
	std::string key;
	std::string display_name;
	int province_id = 0;
};

struct InstalledCountyTitle
{
	std::string key;
	std::string display_name;
	std::string duchy_key;
	std::string kingdom_key;
	std::string empire_key;
	std::vector<InstalledBaronyTitle> baronies;
};

struct InstalledCulture
{
	std::string key;
	std::string display_name;
	std::string ethos;
	std::string heritage;
	std::string language;
	std::vector<std::string> parents;
};

struct InstalledReligion
{
	std::string key;
	std::string display_name;
	std::string family;
};

struct InstalledFaith
{
	std::string key;
	std::string display_name;
	std::string religion_key;
	std::string religion_display_name;
	std::string religion_family;
	std::vector<std::string> doctrines;
};

struct InstalledTitles
{
	std::map<std::string, InstalledTitle> titles;
	std::map<std::string, InstalledCountyTitle> counties;
	std::map<std::string, InstalledCulture> cultures;
	std::map<std::string, InstalledReligion> religions;
	std::map<std::string, InstalledFaith> faiths;
	std::map<std::string, std::string> localizations;

	[[nodiscard]] std::string localizedNameFor(const std::string& title_key) const;
	[[nodiscard]] std::string resolveLocalization(const std::string& value) const;
};

class InstalledTitlesLoader
{
  public:
	InstalledTitles load(const std::filesystem::path& game_path,
		 const std::vector<std::filesystem::path>& mod_paths = {}) const;
};

}  // namespace ck3eu5::ck3
