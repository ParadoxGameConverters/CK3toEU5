#pragma once

#include "config/configuration.h"
#include "diagnostics/diagnostics_report.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ck3eu5::mappers {

struct TitleMapping
{
	std::string ck3_title;
	std::string eu5_tag;
	std::string display_name;
	std::string adjective;
	std::string country_rank;
	int technology_level = -1;
};

struct GovernmentMapping
{
	std::string ck3_government;
	std::string eu5_type = "monarchy";
	int centralization_vs_decentralization = 0;
	int traditionalist_vs_innovative = 0;
	int spiritualist_vs_humanist = 0;
	int aristocracy_vs_plutocracy = 0;
	int serfdom_vs_free_subjects = 0;
	int mercantilism_vs_free_trade = 0;
	int belligerent_vs_conciliatory = 0;
	int quality_vs_quantity = 0;
	int offensive_vs_defensive = 0;
	int land_vs_naval = 0;
	int capital_economy_vs_traditional_economy = 0;
	int individualism_vs_communalism = 0;
	int outward_vs_inward = 0;
};

struct MapperBundle
{
	std::map<std::string, std::vector<std::string>> province_mappings;
	std::map<std::string, TitleMapping> title_mappings;
	std::map<std::string, std::string> culture_mappings;
	std::map<std::string, std::string> religion_mappings;
	std::map<std::string, GovernmentMapping> government_mappings;
	std::set<std::string> reserved_tags;

	[[nodiscard]] std::vector<std::string> mapCountyToLocations(const std::string& county_key) const;
	[[nodiscard]] std::optional<TitleMapping> mapTitle(const std::string& title_key) const;
	[[nodiscard]] std::string mapCulture(const std::string& culture_key) const;
	[[nodiscard]] std::string mapReligion(const std::string& religion_key) const;
	[[nodiscard]] GovernmentMapping mapGovernment(const std::string& government_key) const;
};

class MapperBundleBuilder
{
  public:
	MapperBundle load(const config::Configuration& configuration, diagnostics::DiagnosticsReport& diagnostics) const;
};

}  // namespace ck3eu5::mappers
