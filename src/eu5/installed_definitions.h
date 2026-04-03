#pragma once

#include <set>
#include <string>

namespace ck3eu5::eu5 {

struct InstalledDefinitions
{
	std::set<std::string> cultures;
	std::set<std::string> religions;
	std::set<std::string> government_types;
	std::set<std::string> country_ranks;
	std::set<std::string> pop_types;
	std::set<std::string> heir_selections;
	std::set<std::string> law_values;
	std::set<std::string> government_reforms;
	std::set<std::string> estate_privileges;
	std::set<std::string> subject_types;
	std::set<std::string> building_types;
	std::set<std::string> unit_types;
	std::set<std::string> scripted_relations;
	std::set<std::string> subject_military_stances;
};

}  // namespace ck3eu5::eu5
