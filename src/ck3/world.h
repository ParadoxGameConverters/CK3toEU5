#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ck3eu5::ck3 {

enum class TitleRank
{
	Unknown = -1,
	Barony = 0,
	County = 1,
	Duchy = 2,
	Kingdom = 3,
	Empire = 4
};

TitleRank titleRankFromString(const std::string& value);
TitleRank titleRankFromTitleKey(const std::string& title_key);
std::string toString(TitleRank rank);

struct Character
{
	std::string id;
	std::string first_name;
	std::string last_name;
	std::string dynasty;
	std::string dynasty_house_id;
	std::string culture;
	std::string culture_id;
	std::string faith;
	std::string faith_id;
	std::string government;
	std::string primary_title;
	std::string liege;
	std::string employer_id;
	std::string spouse_id;
	std::string suzerain_id;
	std::string realm_capital_province;
	std::string birth_date = "1300.1.1";
	std::string death_date;
	double gold = 0.0;
	double realm_current_strength = 0.0;
	double realm_max_strength = 0.0;
	double realm_levy = 0.0;
	int adm = 50;
	int dip = 50;
	int mil = 50;
	bool female = false;
	bool dead = false;
	std::vector<std::string> claims;
	std::vector<std::string> domain_titles;
	std::vector<std::string> held_titles;
};

struct Title
{
	std::string key;
	std::string source_id;
	TitleRank rank = TitleRank::Unknown;
	std::string holder_id;
	std::string de_jure_liege_title;
	std::string de_facto_liege_title;
	std::string government;
	std::string capital_county;
	std::string capital_province;
	std::string display_name;
	std::vector<std::string> de_jure_vassals;
	std::vector<std::string> de_facto_vassals;
	std::vector<std::string> owned_de_jure_counties;
	std::vector<std::string> owned_de_facto_counties;
	std::vector<std::string> heirs;
	std::vector<std::string> claimants;
	std::vector<std::string> electors;
	std::vector<std::string> previous_holders;
	bool capital_barony = false;
};

struct WarParticipant
{
	std::string character_id;
	std::string joined_date;
	double contribution_score = 0.0;
};

struct War
{
	std::string id;
	std::string name;
	std::string cb_type;
	std::string start_date;
	std::string attacker_id;
	std::string defender_id;
	std::string claimant_id;
	std::vector<std::string> targeted_titles;
	std::vector<WarParticipant> attackers;
	std::vector<WarParticipant> defenders;
};

struct County
{
	std::string key;
	std::string source_title_id;
	std::string owner_id;
	std::string top_liege_id;
	std::string culture;
	std::string culture_id;
	std::string faith;
	std::string faith_id;
	std::string government;
	std::string province_key;
	std::string display_name;
	std::string terrain;
	int development = 1;
	std::vector<std::string> holdings;
	std::vector<std::string> barony_keys;
	std::vector<std::string> barony_display_names;
	std::vector<std::string> barony_province_keys;
	std::vector<std::string> neighbors;
};

struct Dynasty
{
	std::string id;
	std::string key;
	std::string display_name;
	bool good_for_realm_name = false;
};

struct DynastyHouse
{
	std::string id;
	std::string key;
	std::string name;
	std::string localized_name;
	std::string prefix;
	std::string dynasty_id;
	std::string house_head_id;
};

struct Culture
{
	std::string id;
	std::string key;
	std::string display_name;
	std::string ethos;
	std::string heritage;
	std::string language;
	std::vector<std::string> parents;
};

struct Faith
{
	std::string id;
	std::string key;
	std::string religion;
	std::string display_name;
	std::string religion_display_name;
	std::string religion_family;
	std::vector<std::string> doctrines;
};

struct World
{
	std::string date = "1337.1.1";
	std::map<std::string, Dynasty> dynasties;
	std::map<std::string, DynastyHouse> dynasty_houses;
	std::map<std::string, Culture> cultures;
	std::map<std::string, Faith> faiths;
	std::map<std::string, Character> characters;
	std::map<std::string, Title> titles;
	std::map<std::string, War> wars;
	std::map<std::string, County> counties;

	[[nodiscard]] const Dynasty* getDynasty(const std::string& id) const;
	[[nodiscard]] const DynastyHouse* getDynastyHouse(const std::string& id) const;
	[[nodiscard]] const Culture* getCulture(const std::string& id) const;
	[[nodiscard]] const Faith* getFaith(const std::string& id) const;
	[[nodiscard]] const Character* getCharacter(const std::string& id) const;
	[[nodiscard]] Character* getCharacter(const std::string& id);
	[[nodiscard]] const Title* getTitle(const std::string& key) const;
	[[nodiscard]] const Title* getTitleBySourceId(const std::string& source_id) const;
	[[nodiscard]] const County* getCounty(const std::string& key) const;
	[[nodiscard]] const County* findCountyByBaronyProvince(const std::string& province_id) const;
	[[nodiscard]] std::vector<std::string> heldTitleKeysOfCharacter(const std::string& id) const;
	[[nodiscard]] std::vector<std::string> deJureCountyKeysOfTitle(const std::string& key) const;
	[[nodiscard]] std::vector<std::string> deFactoCountyKeysOfTitle(const std::string& key) const;
	[[nodiscard]] TitleRank primaryRankOfCharacter(const std::string& id) const;
	[[nodiscard]] std::string primaryTitleOfCharacter(const std::string& id) const;
	[[nodiscard]] std::string topLiegeOfCharacter(const std::string& id) const;
};

}  // namespace ck3eu5::ck3
