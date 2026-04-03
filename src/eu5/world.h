#pragma once

#include "eu5/framework.h"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ck3eu5::eu5 {

struct Pop
{
	std::string type;
	std::string culture;
	std::string religion;
	double size = 0.0;
};

struct Dynasty
{
	std::string key;
	std::string loc_name_key;
	std::string home_location;
};

struct Character
{
	std::string key;
	std::string first_name_loc_key;
	std::string last_name_loc_key;
	std::string dynasty_key;
	std::string culture;
	std::string religion;
	std::string birth_date = "1300.1.1";
	std::string birth_location;
	std::string death_date;
	int adm = 50;
	int dip = 50;
	int mil = 50;
	bool female = false;
	std::string tag;
};

struct LocationInstance
{
	std::string key;
	std::string owner_tag;
	std::string culture;
	std::string religion;
	std::string rank = "rural_settlement";
	std::string raw_good = "grain";
	std::string region;
	std::string area;
	std::string province_definition;
	std::string town_setup;
	int control = 75;
	double prosperity = 0.0;
	double development = 0.0;
	bool has_feudalism = true;
	std::vector<Pop> pops;
};

struct Country
{
	std::string tag;
	std::string source_character_id;
	std::string source_title_key;
	std::string display_name;
	std::string adjective;
	std::string capital_location;
	std::string primary_culture;
	std::string primary_religion;
	std::string government_type = "monarchy";
	std::string parliament_type;
	std::string country_rank = "rank_duchy";
	int starting_technology_level = 1;
	int gold = 100;
	int prestige = 25;
	int government_power = 50;
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
	std::string ruler_character_key;
	std::string consort_character_key;
	std::string heir_character_key;
	std::string heir_selection;
	std::map<std::string, std::string> laws;
	std::vector<std::string> reforms;
	std::vector<std::string> privileges;
	std::set<std::string> owned_core_locations;
	std::set<std::string> accepted_cultures;
	std::set<std::string> tolerated_cultures;
	std::set<std::string> discovered_regions;
};

struct SubjectRelation
{
	std::string liege_tag;
	std::string subject_tag;
	std::string subject_type = "vassal";
	std::string subject_military_stance;
	std::string start_date;
};

struct ScriptedRelation
{
	std::string first_tag;
	std::string second_tag;
	std::string type;
	bool mutual = false;
};

struct OpinionRelation
{
	std::string first_tag;
	std::string second_tag;
	std::string type;
};

struct RivalRelation
{
	std::string first_tag;
	std::string second_tag;
};

struct MarketCenter
{
	std::string location;
	std::string owner_tag;
	double score = 0.0;
};

struct BuildingInstance
{
	std::string type;
	std::string location;
	std::string tag;
	int level = 1;
};

struct ForcePlan
{
	std::string key;
	std::string tag;
	std::string branch = "army";
	std::string home_location;
	std::string stance = "normal";
	int levy_estimate = 0;
	int standing_estimate = 0;
};

struct StartForceUnit
{
	std::string type;
	int count = 1;
};

struct StartForce
{
	std::string key;
	std::string tag;
	std::string branch = "army";
	std::string location;
	std::string commander_character_key;
	std::vector<StartForceUnit> units;
	bool wartime = false;
};

struct WarParticipant
{
	std::string tag;
	std::string caller_tag;
	std::string reason = "Scripted";
	std::string which = "alliance";
};

struct War
{
	std::string key;
	bool civil_war = false;
	std::string name = "NORMAL_WAR_NAME";
	int ordinal = 1;
	std::string first_tag;
	std::string second_tag;
	std::string start_date;
	std::string action_date;
	std::string target_location;
	std::vector<WarParticipant> attackers;
	std::vector<WarParticipant> defenders;
};

struct Road
{
	std::string from;
	std::string to;
};

struct World
{
	std::string date = "1337.1.1";
	std::map<std::string, Country> countries;
	std::map<std::string, LocationInstance> locations;
	std::map<std::string, Character> characters;
	std::map<std::string, Dynasty> dynasties;
	std::vector<SubjectRelation> subject_relations;
	std::vector<ScriptedRelation> scripted_relations;
	std::vector<OpinionRelation> opinions;
	std::vector<RivalRelation> rivals;
	std::vector<MarketCenter> markets;
	std::vector<BuildingInstance> buildings;
	std::vector<ForcePlan> force_plans;
	std::vector<StartForce> start_forces;
	std::vector<War> wars;
	std::vector<Road> roads;
	std::map<std::string, std::string> localization;
};

}  // namespace ck3eu5::eu5
