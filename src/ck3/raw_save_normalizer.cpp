#include "ck3/raw_save_normalizer.h"

#include "ck3/world.h"
#include "common/pds_node.h"
#include "common/pds_parser.h"
#include "common/string_utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <rakaly.h>

namespace ck3eu5::ck3 {
namespace {

struct AssignmentView
{
	std::string key;
	std::string_view value;
	size_t next_position = 0;
};

struct RawTitle
{
	std::string id;
	std::string key;
	TitleRank rank = TitleRank::Unknown;
	std::string holder_id;
	std::string de_facto_liege_id;
	std::string government;
	std::string display_name;
	std::string capital_province;
	bool theocratic_lease = false;
	bool capital_barony = false;
	std::vector<std::string> de_jure_vassals;
	std::vector<std::string> heir_ids;
	std::vector<std::string> claimant_ids;
	std::vector<std::string> elector_ids;
	std::vector<std::string> previous_holder_ids;
};

struct RawDynasty
{
	std::string id;
	std::string key;
	bool good_for_realm_name = false;
};

struct RawCharacter
{
	std::string id;
	std::string first_name;
	std::string dynasty_house_id;
	std::string culture_id;
	std::string faith_id;
	std::string government;
	std::string employer_character_id;
	std::string spouse_character_id;
	std::string suzerain_character_id;
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
	std::vector<std::string> claim_title_ids;
	std::vector<std::string> domain_titles;
	std::string primary_title_key;
	std::string liege_character_id;
};

struct RawCounty
{
	std::string key;
	std::string culture_id;
	std::string faith_id;
	int development = 0;
};

struct RawWarParticipant
{
	std::string character_id;
	std::string joined_date;
	double contribution_score = 0.0;
};

struct RawWar
{
	std::string id;
	std::string name;
	std::string cb_type;
	std::string start_date;
	std::string attacker_id;
	std::string defender_id;
	std::string claimant_id;
	std::vector<std::string> targeted_title_ids;
	std::vector<RawWarParticipant> attackers;
	std::vector<RawWarParticipant> defenders;
};

struct RawCulture
{
	std::string id;
	std::string key;
	std::string display_name;
};

struct RawFaith
{
	std::string id;
	std::string key;
	std::string religion_key;
	std::string display_name;
};

struct RawDynastyHouse
{
	std::string id;
	std::string key;
	std::string name;
	std::string localized_name;
	std::string prefix;
	std::string dynasty_id;
	std::string house_head_id;
};

std::optional<AssignmentView> nextAssignmentInBlock(std::string_view block_value, size_t position);

std::string quoteString(std::string_view value)
{
	std::string output;
	output.reserve(value.size() + 2);
	output.push_back('"');
	for (const char ch: value)
	{
		switch (ch)
		{
			case '\\':
				output += "\\\\";
				break;
			case '"':
				output += "\\\"";
				break;
			case '\n':
				output += "\\n";
				break;
			case '\r':
				output += "\\r";
				break;
			case '\t':
				output += "\\t";
				break;
			default:
				output.push_back(ch);
				break;
		}
	}
	output.push_back('"');
	return output;
}

bool isTokenBoundary(const char ch)
{
	return ch == '\0' || std::isspace(static_cast<unsigned char>(ch)) || ch == '=' || ch == '{' || ch == '}' || ch == '#';
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
	throw std::runtime_error("Unterminated quoted token in CK3 save.");
}

size_t parseBareTokenEnd(const std::string_view text, size_t position)
{
	while (position < text.size() && !isTokenBoundary(text[position]))
	{
		++position;
	}
	return position;
}

std::string stripToken(std::string_view token)
{
	return common::stripQuotes(common::trim(token));
}

size_t findMatchingBrace(const std::string_view text, const size_t open_brace_position)
{
	if (open_brace_position >= text.size() || text[open_brace_position] != '{')
	{
		throw std::runtime_error("Internal error: expected opening brace.");
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

	throw std::runtime_error("Unbalanced braces in CK3 save.");
}

std::optional<AssignmentView> nextAssignmentInRoot(const std::string_view text, size_t position)
{
	while (true)
	{
		position = skipWhitespaceAndComments(text, position);
		if (position >= text.size())
		{
			return std::nullopt;
		}

		const size_t key_start = position;
		const size_t key_end = text[position] == '"' ? parseQuotedTokenEnd(text, position) : parseBareTokenEnd(text, position);
		auto key = stripToken(text.substr(key_start, key_end - key_start));
		position = skipWhitespaceAndComments(text, key_end);
		if (position >= text.size())
		{
			return std::nullopt;
		}
		if (text[position] != '=')
		{
			position = key_end;
			continue;
		}

		position = skipWhitespaceAndComments(text, position + 1);
		if (position >= text.size())
		{
			throw std::runtime_error("Missing value while scanning CK3 save.");
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
		}

		return AssignmentView{.key = std::move(key), .value = text.substr(value_start, value_end - value_start), .next_position = value_end};
	}
}

std::optional<std::string_view> findRootAssignmentValue(const std::string_view text, std::string_view target_key)
{
	size_t position = 0;
	while (const auto assignment = nextAssignmentInRoot(text, position))
	{
		if (assignment->key == target_key)
		{
			return assignment->value;
		}
		position = assignment->next_position;
	}
	return std::nullopt;
}

std::optional<std::string_view> findAssignmentValueInBlock(const std::string_view block_value, std::string_view target_key)
{
	size_t position = 1;
	while (const auto assignment = nextAssignmentInBlock(block_value, position))
	{
		if (assignment->key == target_key)
		{
			return assignment->value;
		}
		position = assignment->next_position;
	}
	return std::nullopt;
}

std::optional<AssignmentView> nextAssignmentInBlock(const std::string_view block_value, size_t position)
{
	if (block_value.empty() || block_value.front() != '{' || block_value.back() != '}')
	{
		throw std::runtime_error("Internal error: expected PDS block value.");
	}

	position = skipWhitespaceAndComments(block_value, position);
	if (position >= block_value.size() - 1 || block_value[position] == '}')
	{
		return std::nullopt;
	}

	const size_t key_start = position;
	const size_t key_end = block_value[position] == '"' ? parseQuotedTokenEnd(block_value, position) : parseBareTokenEnd(block_value, position);
	auto key = stripToken(block_value.substr(key_start, key_end - key_start));

	position = skipWhitespaceAndComments(block_value, key_end);
	if (position >= block_value.size() || block_value[position] != '=')
	{
		throw std::runtime_error("Malformed assignment inside CK3 save block.");
	}

	position = skipWhitespaceAndComments(block_value, position + 1);
	if (position >= block_value.size())
	{
		throw std::runtime_error("Missing value inside CK3 save block.");
	}

	const size_t value_start = position;
	size_t value_end = position;
	if (block_value[position] == '{')
	{
		value_end = findMatchingBrace(block_value, position) + 1;
	}
	else if (block_value[position] == '"')
	{
		value_end = parseQuotedTokenEnd(block_value, position);
	}
	else
	{
		value_end = parseBareTokenEnd(block_value, position);
	}

	return AssignmentView{.key = std::move(key), .value = block_value.substr(value_start, value_end - value_start), .next_position = value_end};
}

template <typename Callback>
void forEachAssignmentInBlock(const std::string_view block_value, Callback&& callback)
{
	size_t position = 1;
	while (const auto assignment = nextAssignmentInBlock(block_value, position))
	{
		callback(*assignment);
		position = assignment->next_position;
	}
}

common::PdsNode parseAssignedValue(std::string_view key, std::string_view value)
{
	common::PdsParser parser;
	std::string input;
	input.reserve(key.size() + value.size() + 1);
	input.append(key);
	input.push_back('=');
	input.append(value);
	auto root = parser.parse(input);
	const auto* node = root.get(key);
	if (!node)
	{
		throw std::runtime_error("Internal parser error while reading CK3 save block.");
	}
	return *node;
}

std::string cleanGeneratedName(std::string raw_name)
{
	raw_name = common::stripQuotes(raw_name);
	for (const std::string_view prefix: {"dynn_", "dynnp_", "nick_"})
	{
		if (raw_name.rfind(prefix, 0) == 0)
		{
			raw_name = raw_name.substr(prefix.size());
			break;
		}
	}
	for (char& ch: raw_name)
	{
		if (ch == '_')
		{
			ch = ' ';
		}
	}
	return common::trim(raw_name);
}

std::string deriveDisplayNameFromKey(const std::string& key)
{
	std::string display = key;
	for (const std::string_view prefix: {"b_", "c_", "d_", "k_", "e_"})
	{
		if (display.rfind(prefix, 0) == 0)
		{
			display = display.substr(prefix.size());
			break;
		}
	}
	for (char& ch: display)
	{
		if (ch == '_')
		{
			ch = ' ';
		}
	}
	if (!display.empty())
	{
		display[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(display[0])));
	}
	return display;
}

void appendUniqueValue(std::vector<std::string>& values, const std::string& value)
{
	if (!value.empty() && std::find(values.begin(), values.end(), value) == values.end())
	{
		values.push_back(value);
	}
}

std::vector<const RawTitle*> deriveBaronies(const RawTitle& county_title, const std::map<std::string, RawTitle>& titles)
{
	std::vector<const RawTitle*> baronies;
	for (const auto& barony_id: county_title.de_jure_vassals)
	{
		const auto title_it = titles.find(barony_id);
		if (title_it == titles.end())
		{
			continue;
		}
		if (title_it->second.rank != TitleRank::Barony)
		{
			continue;
		}
		baronies.push_back(&title_it->second);
	}
	return baronies;
}

std::string faithKeyForNode(const common::PdsNode& node)
{
	if (const auto tag = node.getString("tag"); !tag.empty())
	{
		return tag;
	}
	return node.getString("template");
}

std::vector<std::string> deriveHoldings(const RawTitle& county_title, const std::map<std::string, RawTitle>& titles)
{
	std::vector<std::string> holdings;
	holdings.push_back(county_title.government == "tribal_government" ? "tribe" : "castle");

	bool added_city = false;
	bool added_temple = false;
	for (const auto* barony: deriveBaronies(county_title, titles))
	{
		if (barony->theocratic_lease)
		{
			holdings.push_back("temple");
			added_temple = true;
			continue;
		}
		if (!barony->holder_id.empty() && barony->holder_id != county_title.holder_id)
		{
			holdings.push_back("city");
			added_city = true;
			continue;
		}
		if (!barony->capital_barony)
		{
			holdings.push_back(county_title.government == "tribal_government" ? "tribe" : "castle");
		}
	}

	if (!added_city && holdings.size() <= 2 && county_title.government != "tribal_government")
	{
		holdings.push_back("city");
	}
	if (!added_temple && holdings.size() <= 3)
	{
		holdings.push_back("temple");
	}
	return holdings;
}

std::map<std::string, RawDynasty> parseDynastyMap(const std::string_view text)
{
	std::map<std::string, RawDynasty> dynasty_map;
	const auto dynasties_outer = findRootAssignmentValue(text, "dynasties");
	if (!dynasties_outer)
	{
		return dynasty_map;
	}
	const auto dynasties = findAssignmentValueInBlock(*dynasties_outer, "dynasties");
	if (!dynasties)
	{
		return dynasty_map;
	}

	forEachAssignmentInBlock(*dynasties, [&dynasty_map](const AssignmentView& assignment) {
		const auto node = parseAssignedValue("entry", assignment.value);
		auto dynasty = RawDynasty{
			 .id = assignment.key,
			 .key = node.getString("key"),
			 .good_for_realm_name = node.getBool("good_for_realm_name", false),
		};
		if (!dynasty.key.empty())
		{
			dynasty_map[assignment.key] = std::move(dynasty);
		}
	});
	return dynasty_map;
}

std::map<std::string, RawCulture> parseCultureMap(const std::string_view text)
{
	std::map<std::string, RawCulture> culture_map;
	const auto culture_manager = findRootAssignmentValue(text, "culture_manager");
	if (!culture_manager)
	{
		return culture_map;
	}
	const auto cultures = findAssignmentValueInBlock(*culture_manager, "cultures");
	if (!cultures)
	{
		return culture_map;
	}

	forEachAssignmentInBlock(*cultures, [&culture_map](const AssignmentView& assignment) {
		const auto node = parseAssignedValue("entry", assignment.value);
		const auto culture_key = node.getString("culture_template");
		if (!culture_key.empty())
		{
			culture_map[assignment.key] = RawCulture{
				 .id = assignment.key,
				 .key = culture_key,
				 .display_name = deriveDisplayNameFromKey(culture_key),
			};
		}
	});
	return culture_map;
}

std::map<std::string, RawFaith> parseFaithMap(const std::string_view text)
{
	std::map<std::string, RawFaith> faith_map;
	const auto religion = findRootAssignmentValue(text, "religion");
	if (!religion)
	{
		return faith_map;
	}
	std::map<std::string, std::string> religion_keys;
	if (const auto religions = findAssignmentValueInBlock(*religion, "religions"))
	{
		forEachAssignmentInBlock(*religions, [&religion_keys](const AssignmentView& assignment) {
			const auto node = parseAssignedValue("entry", assignment.value);
			const auto religion_key = faithKeyForNode(node);
			if (!religion_key.empty())
			{
				religion_keys[assignment.key] = religion_key;
			}
		});
	}
	const auto faiths = findAssignmentValueInBlock(*religion, "faiths");
	if (!faiths)
	{
		return faith_map;
	}

	forEachAssignmentInBlock(*faiths, [&faith_map, &religion_keys](const AssignmentView& assignment) {
		const auto node = parseAssignedValue("entry", assignment.value);
		const auto faith_key = faithKeyForNode(node);
		if (!faith_key.empty())
		{
			auto faith = RawFaith{
				 .id = assignment.key,
				 .key = faith_key,
				 .religion_key = {},
				 .display_name = deriveDisplayNameFromKey(faith_key),
			};
			const auto religion_id = node.getString("religion");
			if (const auto religion_it = religion_keys.find(religion_id); religion_it != religion_keys.end())
			{
				faith.religion_key = religion_it->second;
			}
			faith_map[assignment.key] = std::move(faith);
		}
	});
	return faith_map;
}

std::map<std::string, RawDynastyHouse> parseDynastyHouseMap(const std::string_view text)
{
	std::map<std::string, RawDynastyHouse> dynasty_house_map;
	const auto dynasties = findRootAssignmentValue(text, "dynasties");
	if (!dynasties)
	{
		return dynasty_house_map;
	}
	const auto houses = findAssignmentValueInBlock(*dynasties, "dynasty_house");
	if (!houses)
	{
		return dynasty_house_map;
	}

	forEachAssignmentInBlock(*houses, [&dynasty_house_map](const AssignmentView& assignment) {
		const auto node = parseAssignedValue("entry", assignment.value);
		auto dynasty_name = cleanGeneratedName(node.getString("name"));
		const auto prefix = cleanGeneratedName(node.getString("prefix"));
		const auto localized_name = cleanGeneratedName(node.getString("localized_name"));
		if (!prefix.empty())
		{
			dynasty_name = prefix + " " + dynasty_name;
		}
		if (!dynasty_name.empty() || !localized_name.empty() || !node.getString("key").empty())
		{
			dynasty_house_map[assignment.key] = RawDynastyHouse{
				 .id = assignment.key,
				 .key = node.getString("key"),
				 .name = dynasty_name.empty() ? localized_name : dynasty_name,
				 .localized_name = localized_name,
				 .prefix = prefix,
				 .dynasty_id = node.getString("dynasty"),
				 .house_head_id = node.getString("head_of_house"),
			};
		}
	});
	return dynasty_house_map;
}

std::map<std::string, RawTitle> parseTitles(const std::string_view text)
{
	std::map<std::string, RawTitle> titles;
	const auto landed_titles_outer = findRootAssignmentValue(text, "landed_titles");
	if (!landed_titles_outer)
	{
		return titles;
	}
	const auto landed_titles = findAssignmentValueInBlock(*landed_titles_outer, "landed_titles");
	if (!landed_titles)
	{
		return titles;
	}

	forEachAssignmentInBlock(*landed_titles, [&titles](const AssignmentView& assignment) {
		const auto node = parseAssignedValue("entry", assignment.value);
		RawTitle title;
		title.id = assignment.key;
		title.key = node.getString("key");
		if (title.key.empty())
		{
			return;
		}
		title.rank = titleRankFromTitleKey(title.key);
		title.holder_id = node.getString("holder");
		title.de_facto_liege_id = node.getString("de_facto_liege");
		title.government = node.getString("history_government");
		title.display_name = node.getString("name");
		if (title.display_name.empty())
		{
			title.display_name = deriveDisplayNameFromKey(title.key);
		}
		title.capital_province = node.getString("capital");
		title.theocratic_lease = node.getBool("theocratic_lease", false);
		title.capital_barony = node.getBool("capital_barony", false);
		title.de_jure_vassals = node.getListOfScalars("de_jure_vassals");
		title.heir_ids = node.getListOfScalars("heir");
		title.claimant_ids = node.getListOfScalars("claim");
		if (const auto* succession_election = node.get("succession_election"))
		{
			title.elector_ids = succession_election->getListOfScalars("electors");
		}
		if (title.elector_ids.empty())
		{
			title.elector_ids = node.getListOfScalars("electors");
		}
		if (const auto* history = node.get("history"))
		{
			for (const auto& [date_key, entry]: history->properties())
			{
				(void)date_key;
				if (entry.isScalar())
				{
					appendUniqueValue(title.previous_holder_ids, entry.scalarOr());
				}
			}
		}
		titles[title.id] = std::move(title);
	});
	return titles;
}

void parseCharacterSection(const std::string_view block_value, const bool dead_section, std::map<std::string, RawCharacter>& characters)
{
	forEachAssignmentInBlock(block_value, [&characters, dead_section](const AssignmentView& assignment) {
		const auto node = parseAssignedValue("entry", assignment.value);

		RawCharacter character;
		character.id = assignment.key;
		character.first_name = node.getString("first_name");
		character.dynasty_house_id = node.getString("dynasty_house");
		character.culture_id = node.getString("culture");
		character.faith_id = node.getString("faith");
		character.birth_date = node.getString("birth", "1300.1.1");
		character.female = node.getBool("female", false);
		character.dead = dead_section;

		if (const auto* alive_data = node.get("alive_data"))
		{
			character.gold = alive_data->getDouble("gold", 0.0);
			if (const auto* landed_data = alive_data->get("landed_data"))
			{
				character.government = landed_data->getString("government");
				character.domain_titles = landed_data->getListOfScalars("domain");
				character.realm_capital_province = landed_data->getString("realm_capital");
				character.realm_current_strength = landed_data->getDouble("current_strength", 0.0);
				character.realm_max_strength = landed_data->getDouble("strength", 0.0);
				character.realm_levy = landed_data->getDouble("levy", 0.0);
			}
			if (const auto* court_data = alive_data->get("court_data"))
			{
				character.employer_character_id = court_data->getString("employer");
			}
			character.suzerain_character_id = alive_data->getString("obedience_target");
			for (const auto* claim: alive_data->getAll("claim"))
			{
				const auto claim_title_id = claim->getString("title");
				if (!claim_title_id.empty())
				{
					character.claim_title_ids.push_back(claim_title_id);
				}
			}
		}

		if (const auto* dead_data = node.get("dead_data"))
		{
			character.dead = true;
			character.death_date = dead_data->getString("date");
			if (character.death_date.empty())
			{
				character.death_date = dead_data->getString("death_date");
			}
		}

		if (const auto* family_data = node.get("family_data"))
		{
			character.spouse_character_id = family_data->getString("primary_spouse");
		}

		const auto skills = node.getListOfScalars("skill");
		if (skills.size() >= 3)
		{
			character.dip = common::parseInt(skills[0]).value_or(50);
			character.mil = common::parseInt(skills[1]).value_or(50);
			character.adm = common::parseInt(skills[2]).value_or(50);
		}
		const auto existing = characters.find(character.id);
		if (existing == characters.end() || (!character.dead && existing->second.dead))
		{
			characters[character.id] = std::move(character);
		}
	});
}

std::map<std::string, RawCharacter> parseCharacters(const std::string_view text)
{
	std::map<std::string, RawCharacter> characters;
	if (const auto living = findRootAssignmentValue(text, "living"))
	{
		parseCharacterSection(*living, false, characters);
	}
	if (const auto dead_unprunable = findRootAssignmentValue(text, "dead_unprunable"))
	{
		parseCharacterSection(*dead_unprunable, true, characters);
	}
	if (const auto dead_prunable = findRootAssignmentValue(text, "dead_prunable"))
	{
		parseCharacterSection(*dead_prunable, true, characters);
	}
	return characters;
}

std::map<std::string, RawCounty> parseCounties(const std::string_view text)
{
	std::map<std::string, RawCounty> counties;
	const auto county_manager = findRootAssignmentValue(text, "county_manager");
	if (!county_manager)
	{
		return counties;
	}
	const auto county_block = findAssignmentValueInBlock(*county_manager, "counties");
	if (!county_block)
	{
		return counties;
	}

	forEachAssignmentInBlock(*county_block, [&counties](const AssignmentView& assignment) {
		if (titleRankFromTitleKey(assignment.key) != TitleRank::County)
		{
			return;
		}
		const auto node = parseAssignedValue("entry", assignment.value);
		RawCounty county;
		county.key = assignment.key;
		county.culture_id = node.getString("culture");
		county.faith_id = node.getString("faith");
		county.development = node.getInt("development", 0);
		counties[county.key] = std::move(county);
	});
	return counties;
}

std::map<std::string, RawWar> parseWars(const std::string_view text)
{
	std::map<std::string, RawWar> wars;
	const auto wars_root = findRootAssignmentValue(text, "wars");
	if (!wars_root)
	{
		return wars;
	}

	const auto active_wars = findAssignmentValueInBlock(*wars_root, "active_wars");
	if (!active_wars)
	{
		return wars;
	}

	forEachAssignmentInBlock(*active_wars, [&wars](const AssignmentView& assignment) {
		if (stripToken(assignment.value) == "none")
		{
			return;
		}

		const auto node = parseAssignedValue("entry", assignment.value);
		RawWar war;
		war.id = assignment.key;
		war.name = node.getString("name");
		war.start_date = node.getString("start_date");

		if (const auto* casus_belli = node.get("casus_belli"))
		{
			war.cb_type = casus_belli->getString("type");
			war.attacker_id = casus_belli->getString("attacker");
			war.defender_id = casus_belli->getString("defender");
			war.claimant_id = casus_belli->getString("claimant");
			war.targeted_title_ids = casus_belli->getListOfScalars("targeted_titles");
		}

		auto parse_side = [](const common::PdsNode* side_node, std::vector<RawWarParticipant>& participants) {
			if (!side_node)
			{
				return;
			}
			const auto* participant_list = side_node->get("participants");
			if (!participant_list)
			{
				return;
			}
			for (const auto& participant_node: participant_list->items())
			{
				RawWarParticipant participant;
				participant.character_id = participant_node.getString("character");
				participant.joined_date = participant_node.getString("date");
				if (const auto* contribution = participant_node.get("contribution"))
				{
					for (const auto& item: contribution->items())
					{
						participant.contribution_score += item.asDouble(0.0);
					}
				}
				if (!participant.character_id.empty())
				{
					participants.push_back(std::move(participant));
				}
			}
		};

		parse_side(node.get("attacker"), war.attackers);
		parse_side(node.get("defender"), war.defenders);
		if (war.attacker_id.empty() && !war.attackers.empty())
		{
			war.attacker_id = war.attackers.front().character_id;
		}
		if (war.defender_id.empty() && !war.defenders.empty())
		{
			war.defender_id = war.defenders.front().character_id;
		}
		if (!war.start_date.empty() && (!war.attackers.empty() || !war.defenders.empty()))
		{
			wars[war.id] = std::move(war);
		}
	});

	return wars;
}

std::string findSaveDate(const std::string_view text)
{
	if (const auto date = findRootAssignmentValue(text, "date"))
	{
		return stripToken(*date);
	}
	return "1337.1.1";
}

void choosePrimaryTitles(std::map<std::string, RawCharacter>& characters, const std::map<std::string, RawTitle>& titles)
{
	for (auto& [character_id, character]: characters)
	{
		auto pick_title = [&](const std::string& title_id) {
			const auto title_it = titles.find(title_id);
			if (title_it == titles.end())
			{
				return;
			}
			if (title_it->second.rank < TitleRank::County)
			{
				return;
			}
			if (character.primary_title_key.empty())
			{
				character.primary_title_key = title_it->second.key;
				return;
			}
			const auto current_rank = titleRankFromTitleKey(character.primary_title_key);
			if (title_it->second.rank > current_rank)
			{
				character.primary_title_key = title_it->second.key;
			}
		};

		for (const auto& domain_title_id: character.domain_titles)
		{
			pick_title(domain_title_id);
		}
		if (!character.primary_title_key.empty())
		{
			continue;
		}
		for (const auto& [title_id, title]: titles)
		{
			if (title.holder_id == character_id)
			{
				pick_title(title_id);
			}
		}
	}
}

void deriveCharacterLieges(std::map<std::string, RawCharacter>& characters, const std::map<std::string, RawTitle>& titles)
{
	for (auto& [character_id, character]: characters)
	{
		if (character.primary_title_key.empty())
		{
			continue;
		}
		const auto title_it = std::find_if(titles.begin(), titles.end(), [&](const auto& item) {
			return item.second.key == character.primary_title_key;
		});
		if (title_it == titles.end() || title_it->second.de_facto_liege_id.empty())
		{
			continue;
		}
		const auto liege_title_it = titles.find(title_it->second.de_facto_liege_id);
		if (liege_title_it == titles.end())
		{
			continue;
		}
		if (!liege_title_it->second.holder_id.empty() && liege_title_it->second.holder_id != character_id)
		{
			character.liege_character_id = liege_title_it->second.holder_id;
		}
	}
}

std::string topLiegeForCharacter(const std::map<std::string, RawCharacter>& characters, const std::string& character_id)
{
	std::string current = character_id;
	std::string last = character_id;
	size_t guard = 0;
	while (!current.empty() && guard++ < 128)
	{
		const auto character_it = characters.find(current);
		if (character_it == characters.end() || character_it->second.liege_character_id.empty())
		{
			return last;
		}
		last = character_it->second.liege_character_id;
		current = character_it->second.liege_character_id;
	}
	return last;
}

std::string cultureKeyOrFallback(const std::map<std::string, RawCulture>& mapping, const std::string& id, std::string fallback)
{
	if (const auto itr = mapping.find(id); itr != mapping.end() && !itr->second.key.empty())
	{
		return itr->second.key;
	}
	return fallback.empty() ? "unknown" : fallback;
}

std::string faithKeyOrFallback(const std::map<std::string, RawFaith>& mapping, const std::string& id, std::string fallback)
{
	if (const auto itr = mapping.find(id); itr != mapping.end() && !itr->second.key.empty())
	{
		return itr->second.key;
	}
	return fallback.empty() ? "unknown" : fallback;
}

std::string dynastyNameOrFallback(const std::map<std::string, RawDynastyHouse>& mapping, const std::string& id, std::string fallback)
{
	if (const auto itr = mapping.find(id); itr != mapping.end() && !itr->second.name.empty())
	{
		return itr->second.name;
	}
	return fallback.empty() ? "unknown" : fallback;
}

std::string rankToString(const TitleRank rank)
{
	switch (rank)
	{
		case TitleRank::Barony:
			return "barony";
		case TitleRank::County:
			return "county";
		case TitleRank::Duchy:
			return "duchy";
		case TitleRank::Kingdom:
			return "kingdom";
		case TitleRank::Empire:
			return "empire";
		case TitleRank::Unknown:
			break;
	}
	return "county";
}

std::string readBinaryFile(const std::filesystem::path& path)
{
	std::ifstream input(path, std::ios::binary);
	if (!input.is_open())
	{
		throw std::runtime_error("Could not open CK3 save: " + path.string());
	}
	std::ostringstream buffer;
	buffer << input.rdbuf();
	return buffer.str();
}

}  // namespace

std::string normalizeMeltedSave(const std::string_view melted_save_text)
{
	std::map<std::string, RawDynasty> dynasty_roots;
	std::map<std::string, RawCulture> cultures;
	std::map<std::string, RawFaith> faiths;
	std::map<std::string, RawDynastyHouse> dynasties;
	std::map<std::string, RawTitle> titles;
	std::map<std::string, RawCharacter> characters;
	std::map<std::string, RawWar> wars;
	std::map<std::string, RawCounty> counties;

	try
	{
		dynasty_roots = parseDynastyMap(melted_save_text);
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string("Failed to parse CK3 dynasties: ") + exception.what());
	}
	try
	{
		cultures = parseCultureMap(melted_save_text);
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string("Failed to parse CK3 cultures: ") + exception.what());
	}
	try
	{
		faiths = parseFaithMap(melted_save_text);
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string("Failed to parse CK3 faiths: ") + exception.what());
	}
	try
	{
		dynasties = parseDynastyHouseMap(melted_save_text);
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string("Failed to parse CK3 dynasty houses: ") + exception.what());
	}
	try
	{
		titles = parseTitles(melted_save_text);
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string("Failed to parse CK3 titles: ") + exception.what());
	}
	try
	{
		characters = parseCharacters(melted_save_text);
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string("Failed to parse CK3 living characters: ") + exception.what());
	}
	try
	{
		counties = parseCounties(melted_save_text);
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string("Failed to parse CK3 counties: ") + exception.what());
	}
	try
	{
		wars = parseWars(melted_save_text);
	}
	catch (const std::exception& exception)
	{
		throw std::runtime_error(std::string("Failed to parse CK3 wars: ") + exception.what());
	}

	if (titles.empty() || characters.empty() || counties.empty())
	{
		throw std::runtime_error("Raw CK3 save normalization could not find the required title, character, and county sections.");
	}

	choosePrimaryTitles(characters, titles);
	deriveCharacterLieges(characters, titles);

	std::map<std::string, std::string> county_key_by_capital_province;
	for (const auto& [title_id, title]: titles)
	{
		if (title.rank == TitleRank::County && !title.capital_province.empty())
		{
			county_key_by_capital_province[title.capital_province] = title.key;
		}
	}
	std::map<std::string, std::string> de_jure_liege_key_by_title_key;
	for (const auto& [title_id, title]: titles)
	{
		for (const auto& de_jure_vassal_id: title.de_jure_vassals)
		{
			if (const auto vassal_it = titles.find(de_jure_vassal_id); vassal_it != titles.end())
			{
				de_jure_liege_key_by_title_key[vassal_it->second.key] = title.key;
			}
		}
	}

	std::ostringstream output;
	output << "world = {\n";
	output << "\tdate = " << findSaveDate(melted_save_text) << '\n';

	output << "\tdynasties = {\n";
	for (const auto& [dynasty_id, dynasty]: dynasty_roots)
	{
		output << "\t\t" << dynasty_id << " = {\n";
		output << "\t\t\tkey = " << quoteString(dynasty.key) << '\n';
		if (dynasty.good_for_realm_name)
		{
			output << "\t\t\tgood_for_realm_name = yes\n";
		}
		output << "\t\t}\n";
	}
	output << "\t}\n";

	output << "\tdynasty_houses = {\n";
	for (const auto& [house_id, dynasty_house]: dynasties)
	{
		output << "\t\t" << house_id << " = {\n";
		if (!dynasty_house.key.empty())
		{
			output << "\t\t\tkey = " << quoteString(dynasty_house.key) << '\n';
		}
		output << "\t\t\tname = " << quoteString(dynasty_house.name) << '\n';
		if (!dynasty_house.localized_name.empty())
		{
			output << "\t\t\tlocalized_name = " << quoteString(dynasty_house.localized_name) << '\n';
		}
		if (!dynasty_house.prefix.empty())
		{
			output << "\t\t\tprefix = " << quoteString(dynasty_house.prefix) << '\n';
		}
		if (!dynasty_house.dynasty_id.empty())
		{
			output << "\t\t\tdynasty_id = " << dynasty_house.dynasty_id << '\n';
		}
		if (!dynasty_house.house_head_id.empty())
		{
			output << "\t\t\thouse_head_id = " << dynasty_house.house_head_id << '\n';
		}
		output << "\t\t}\n";
	}
	output << "\t}\n";

	output << "\tcultures = {\n";
	for (const auto& [culture_id, culture]: cultures)
	{
		output << "\t\t" << culture_id << " = {\n";
		output << "\t\t\tkey = " << culture.key << '\n';
		output << "\t\t\tdisplay_name = " << quoteString(culture.display_name) << '\n';
		output << "\t\t}\n";
	}
	output << "\t}\n";

	output << "\tfaiths = {\n";
	for (const auto& [faith_id, faith]: faiths)
	{
		output << "\t\t" << faith_id << " = {\n";
		output << "\t\t\tkey = " << faith.key << '\n';
		if (!faith.religion_key.empty())
		{
			output << "\t\t\treligion = " << faith.religion_key << '\n';
		}
		output << "\t\t\tdisplay_name = " << quoteString(faith.display_name) << '\n';
		output << "\t\t}\n";
	}
	output << "\t}\n";

	output << "\tcharacters = {\n";
	for (const auto& [character_id, character]: characters)
	{
		output << "\t\t" << character_id << " = {\n";
		output << "\t\t\tfirst_name = " << quoteString(character.first_name.empty() ? ("Character " + character_id) : character.first_name) << '\n';
		if (!character.dynasty_house_id.empty())
		{
			output << "\t\t\tdynasty_house_id = " << character.dynasty_house_id << '\n';
		}
		if (const auto dynasty_name = dynastyNameOrFallback(dynasties, character.dynasty_house_id, {}); dynasty_name != "unknown")
		{
			output << "\t\t\tdynasty = " << quoteString(dynasty_name) << '\n';
		}
		if (!character.culture_id.empty())
		{
			output << "\t\t\tculture_id = " << character.culture_id << '\n';
		}
		output << "\t\t\tculture = " << cultureKeyOrFallback(cultures, character.culture_id, "generic") << '\n';
		if (!character.faith_id.empty())
		{
			output << "\t\t\tfaith_id = " << character.faith_id << '\n';
		}
		output << "\t\t\tfaith = " << faithKeyOrFallback(faiths, character.faith_id, "generic_faith") << '\n';
		if (!character.government.empty())
		{
			output << "\t\t\tgovernment = " << character.government << '\n';
		}
		if (!character.primary_title_key.empty())
		{
			output << "\t\t\tprimary_title = " << character.primary_title_key << '\n';
		}
		if (!character.liege_character_id.empty())
		{
			output << "\t\t\tliege = " << character.liege_character_id << '\n';
		}
		if (!character.employer_character_id.empty())
		{
			output << "\t\t\temployer = " << character.employer_character_id << '\n';
		}
		if (!character.spouse_character_id.empty())
		{
			output << "\t\t\tspouse = " << character.spouse_character_id << '\n';
		}
		if (!character.suzerain_character_id.empty())
		{
			output << "\t\t\tsuzerain = " << character.suzerain_character_id << '\n';
		}
		if (character.dead)
		{
			output << "\t\t\tdead = yes\n";
		}
		if (!character.realm_capital_province.empty())
		{
			output << "\t\t\trealm_capital_province = " << character.realm_capital_province << '\n';
		}
		if (!character.domain_titles.empty())
		{
			output << "\t\t\tdomain_titles = {";
			for (const auto& domain_title_id: character.domain_titles)
			{
				if (const auto title_it = titles.find(domain_title_id); title_it != titles.end())
				{
					output << ' ' << title_it->second.key;
				}
			}
			output << " }\n";
		}
		if (!character.claim_title_ids.empty())
		{
			output << "\t\t\tclaims = {";
			for (const auto& claim_title_id: character.claim_title_ids)
			{
				if (const auto title_it = titles.find(claim_title_id); title_it != titles.end())
				{
					output << ' ' << title_it->second.key;
				}
			}
			output << " }\n";
		}
		output << "\t\t\tbirth_date = " << character.birth_date << '\n';
		if (!character.death_date.empty())
		{
			output << "\t\t\tdeath_date = " << character.death_date << '\n';
		}
		output << "\t\t\tgold = " << character.gold << '\n';
		output << "\t\t\trealm_current_strength = " << character.realm_current_strength << '\n';
		output << "\t\t\trealm_max_strength = " << character.realm_max_strength << '\n';
		output << "\t\t\trealm_levy = " << character.realm_levy << '\n';
		output << "\t\t\tadm = " << character.adm << '\n';
		output << "\t\t\tdip = " << character.dip << '\n';
		output << "\t\t\tmil = " << character.mil << '\n';
		output << "\t\t\tfemale = " << (character.female ? "yes" : "no") << '\n';
		output << "\t\t}\n";
	}
	output << "\t}\n";

	output << "\ttitles = {\n";
	for (const auto& [title_id, title]: titles)
	{
		output << "\t\t" << title.key << " = {\n";
		output << "\t\t\tsource_id = " << title_id << '\n';
		output << "\t\t\trank = " << rankToString(title.rank) << '\n';
		if (!title.holder_id.empty())
		{
			output << "\t\t\tholder = " << title.holder_id << '\n';
		}
		if (const auto de_jure_liege_it = de_jure_liege_key_by_title_key.find(title.key); de_jure_liege_it != de_jure_liege_key_by_title_key.end())
		{
			output << "\t\t\tde_jure_liege_title = " << de_jure_liege_it->second << '\n';
		}
		if (!title.de_facto_liege_id.empty())
		{
			if (const auto liege_it = titles.find(title.de_facto_liege_id); liege_it != titles.end())
			{
				output << "\t\t\tde_facto_liege_title = " << liege_it->second.key << '\n';
			}
		}
		if (!title.government.empty())
		{
			output << "\t\t\tgovernment = " << title.government << '\n';
		}
		if (!title.capital_province.empty())
		{
			output << "\t\t\tcapital_province = " << title.capital_province << '\n';
			const auto county_capital_it = county_key_by_capital_province.find(title.capital_province);
			if (county_capital_it != county_key_by_capital_province.end())
			{
				output << "\t\t\tcapital_county = " << county_capital_it->second << '\n';
			}
			else if (title.rank == TitleRank::County)
			{
				output << "\t\t\tcapital_county = " << title.key << '\n';
			}
		}
		if (!title.de_jure_vassals.empty())
		{
			output << "\t\t\tde_jure_vassals = {";
			for (const auto& de_jure_vassal_id: title.de_jure_vassals)
			{
				if (const auto vassal_it = titles.find(de_jure_vassal_id); vassal_it != titles.end())
				{
					output << ' ' << vassal_it->second.key;
				}
			}
			output << " }\n";
		}
		if (!title.heir_ids.empty())
		{
			output << "\t\t\theirs = {";
			for (const auto& heir_id: title.heir_ids)
			{
				output << ' ' << heir_id;
			}
			output << " }\n";
		}
		if (!title.claimant_ids.empty())
		{
			output << "\t\t\tclaimants = {";
			for (const auto& claimant_id: title.claimant_ids)
			{
				output << ' ' << claimant_id;
			}
			output << " }\n";
		}
		if (!title.elector_ids.empty())
		{
			output << "\t\t\telectors = {";
			for (const auto& elector_id: title.elector_ids)
			{
				output << ' ' << elector_id;
			}
			output << " }\n";
		}
		if (!title.previous_holder_ids.empty())
		{
			output << "\t\t\tprevious_holders = {";
			for (const auto& previous_holder_id: title.previous_holder_ids)
			{
				output << ' ' << previous_holder_id;
			}
			output << " }\n";
		}
		if (title.capital_barony)
		{
			output << "\t\t\tcapital_barony = yes\n";
		}
		output << "\t\t\tdisplay_name = " << quoteString(title.display_name) << '\n';
		output << "\t\t}\n";
	}
	output << "\t}\n";

	output << "\twars = {\n";
	for (const auto& [war_id, war]: wars)
	{
		output << "\t\t" << war_id << " = {\n";
		if (!war.name.empty())
		{
			output << "\t\t\tname = " << quoteString(war.name) << '\n';
		}
		if (!war.cb_type.empty())
		{
			output << "\t\t\tcb_type = " << war.cb_type << '\n';
		}
		if (!war.start_date.empty())
		{
			output << "\t\t\tstart_date = " << war.start_date << '\n';
		}
		if (!war.attacker_id.empty())
		{
			output << "\t\t\tattacker = " << war.attacker_id << '\n';
		}
		if (!war.defender_id.empty())
		{
			output << "\t\t\tdefender = " << war.defender_id << '\n';
		}
		if (!war.claimant_id.empty() && war.claimant_id != "4294967295")
		{
			output << "\t\t\tclaimant = " << war.claimant_id << '\n';
		}
		if (!war.targeted_title_ids.empty())
		{
			output << "\t\t\ttargeted_titles = {";
			for (const auto& targeted_title_id: war.targeted_title_ids)
			{
				if (const auto title_it = titles.find(targeted_title_id); title_it != titles.end())
				{
					output << ' ' << title_it->second.key;
				}
			}
			output << " }\n";
		}
		auto write_participants = [&output](std::string_view label, const std::vector<RawWarParticipant>& participants) {
			output << "\t\t\t" << label << " = {\n";
			output << "\t\t\t\tparticipants = {\n";
			for (const auto& participant: participants)
			{
				output << "\t\t\t\t\t{\n";
				output << "\t\t\t\t\t\tcharacter = " << participant.character_id << '\n';
				if (!participant.joined_date.empty())
				{
					output << "\t\t\t\t\t\tdate = " << participant.joined_date << '\n';
				}
				output << "\t\t\t\t\t\tcontribution_score = " << participant.contribution_score << '\n';
				output << "\t\t\t\t\t}\n";
			}
			output << "\t\t\t\t}\n";
			output << "\t\t\t}\n";
		};
		write_participants("attacker", war.attackers);
		write_participants("defender", war.defenders);
		output << "\t\t}\n";
	}
	output << "\t}\n";

	output << "\tcounties = {\n";
	for (const auto& [county_key, county]: counties)
	{
		output << "\t\t" << county_key << " = {\n";
		const auto county_title_it = std::find_if(titles.begin(), titles.end(), [&](const auto& item) {
			return item.second.key == county_key;
		});
		if (county_title_it != titles.end())
		{
			output << "\t\t\tsource_title_id = " << county_title_it->first << '\n';
		}
		if (county_title_it != titles.end() && !county_title_it->second.holder_id.empty())
		{
			output << "\t\t\towner = " << county_title_it->second.holder_id << '\n';
			output << "\t\t\ttop_liege = " << topLiegeForCharacter(characters, county_title_it->second.holder_id) << '\n';
			const auto owner_it = characters.find(county_title_it->second.holder_id);
			if (owner_it != characters.end() && !owner_it->second.government.empty())
			{
				output << "\t\t\tgovernment = " << owner_it->second.government << '\n';
			}
			else if (!county_title_it->second.government.empty())
			{
				output << "\t\t\tgovernment = " << county_title_it->second.government << '\n';
			}
		}
		if (!county.culture_id.empty())
		{
			output << "\t\t\tculture_id = " << county.culture_id << '\n';
		}
		output << "\t\t\tculture = " << cultureKeyOrFallback(cultures, county.culture_id, "generic") << '\n';
		if (!county.faith_id.empty())
		{
			output << "\t\t\tfaith_id = " << county.faith_id << '\n';
		}
		output << "\t\t\tfaith = " << faithKeyOrFallback(faiths, county.faith_id, "generic_faith") << '\n';
		output << "\t\t\tprovince = " << county_key << '\n';
		if (county_title_it != titles.end())
		{
			output << "\t\t\tdisplay_name = " << quoteString(county_title_it->second.display_name) << '\n';
			const auto holdings = deriveHoldings(county_title_it->second, titles);
			const auto baronies = deriveBaronies(county_title_it->second, titles);
			output << "\t\t\tdevelopment = " << std::max(1, county.development + static_cast<int>(holdings.size())) << '\n';
			output << "\t\t\tholdings = {";
			for (const auto& holding: holdings)
			{
				output << ' ' << holding;
			}
			output << " }\n";
			if (!baronies.empty())
			{
				output << "\t\t\tbaronies = {";
				for (const auto* barony: baronies)
				{
					output << ' ' << barony->key;
				}
				output << " }\n";
				output << "\t\t\tbarony_display_names = {";
				for (const auto* barony: baronies)
				{
					output << ' ' << quoteString(barony->display_name);
				}
				output << " }\n";
				output << "\t\t\tbarony_provinces = {";
				for (const auto* barony: baronies)
				{
					output << ' ' << (barony->capital_province.empty() ? "0" : barony->capital_province);
				}
				output << " }\n";
			}
		}
		else
		{
			output << "\t\t\tdisplay_name = " << quoteString(deriveDisplayNameFromKey(county_key)) << '\n';
			output << "\t\t\tdevelopment = " << std::max(1, county.development + 1) << '\n';
			output << "\t\t\tholdings = { castle city temple }\n";
		}
		output << "\t\t}\n";
	}
	output << "\t}\n";
	output << "}\n";

	return output.str();
}

std::string normalizeSaveFile(const std::filesystem::path& save_path)
{
	auto raw_save = readBinaryFile(save_path);
	const auto save = rakaly::parseCk3(raw_save);
	const auto melted = save.melt();
	melted.writeData(raw_save);
	return normalizeMeltedSave(raw_save);
}

}  // namespace ck3eu5::ck3
