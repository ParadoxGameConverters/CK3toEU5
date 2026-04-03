#include "eu5/world_validator.h"

#include <map>
#include <string>

namespace ck3eu5::eu5 {
namespace {

struct UnknownEntry
{
	size_t count = 0;
	std::string sample;
};

struct ParsedDate
{
	int year = 0;
	int month = 0;
	int day = 0;
};

ParsedDate parseDate(const std::string& text)
{
	ParsedDate parsed;
	if (text.empty())
	{
		return parsed;
	}

	size_t start = 0;
	size_t end = text.find('.');
	if (end != std::string::npos)
	{
		parsed.year = std::stoi(text.substr(start, end - start));
		start = end + 1;
		end = text.find('.', start);
		if (end != std::string::npos)
		{
			parsed.month = std::stoi(text.substr(start, end - start));
			start = end + 1;
			parsed.day = std::stoi(text.substr(start));
		}
	}
	return parsed;
}

bool isDateAfter(const std::string& lhs, const std::string& rhs)
{
	if (lhs.empty() || rhs.empty())
	{
		return false;
	}

	const auto left = parseDate(lhs);
	const auto right = parseDate(rhs);
	if (left.year != right.year)
	{
		return left.year > right.year;
	}
	if (left.month != right.month)
	{
		return left.month > right.month;
	}
	return left.day > right.day;
}

template <typename Predicate>
void recordIfUnknown(const std::string& value,
	 const std::string& sample,
	 const Predicate& exists,
	 std::map<std::string, UnknownEntry>& unknowns)
{
	if (value.empty() || exists(value))
	{
		return;
	}

	auto& entry = unknowns[value];
	++entry.count;
	if (entry.sample.empty())
	{
		entry.sample = sample;
	}
}

void flushUnknowns(const std::string& code_prefix,
	 const std::string& label,
	 const std::map<std::string, UnknownEntry>& unknowns,
	 diagnostics::DiagnosticsReport& diagnostics)
{
	for (const auto& [value, entry]: unknowns)
	{
		diagnostics.warning(code_prefix, "Unknown EU5 " + label + " \"" + value + "\" referenced " + std::to_string(entry.count) +
												" time(s); sample: " + entry.sample);
	}
}

}  // namespace

void WorldValidator::validate(const World& world,
	 const InstalledDefinitions& definitions,
	 diagnostics::DiagnosticsReport& diagnostics) const
{
	std::map<std::string, UnknownEntry> unknown_cultures;
	std::map<std::string, UnknownEntry> unknown_religions;
	std::map<std::string, UnknownEntry> unknown_governments;
	std::map<std::string, UnknownEntry> unknown_country_ranks;
	std::map<std::string, UnknownEntry> unknown_pop_types;
	std::map<std::string, UnknownEntry> unknown_heir_selections;
	std::map<std::string, UnknownEntry> unknown_laws;
	std::map<std::string, UnknownEntry> unknown_reforms;
	std::map<std::string, UnknownEntry> unknown_privileges;
	std::map<std::string, UnknownEntry> unknown_subject_types;
	std::map<std::string, UnknownEntry> unknown_buildings;
	std::map<std::string, UnknownEntry> unknown_units;
	std::map<std::string, UnknownEntry> unknown_scripted_relations;
	std::map<std::string, UnknownEntry> unknown_subject_stances;

	for (const auto& [tag, country]: world.countries)
	{
		recordIfUnknown(country.primary_culture,
			 "country " + tag,
			 [&definitions](const std::string& value) { return definitions.cultures.contains(value); },
			 unknown_cultures);
		recordIfUnknown(country.primary_religion,
			 "country " + tag,
			 [&definitions](const std::string& value) { return definitions.religions.contains(value); },
			 unknown_religions);
		recordIfUnknown(country.government_type,
			 "country " + tag,
			 [&definitions](const std::string& value) { return definitions.government_types.contains(value); },
			 unknown_governments);
		recordIfUnknown(country.country_rank,
			 "country " + tag,
			 [&definitions](const std::string& value) { return definitions.country_ranks.contains(value); },
			 unknown_country_ranks);
		recordIfUnknown(country.heir_selection,
			 "country " + tag,
			 [&definitions](const std::string& value) { return definitions.heir_selections.contains(value); },
			 unknown_heir_selections);
		for (const auto& [law_type, law_value]: country.laws)
		{
			recordIfUnknown(law_value,
				 "country " + tag + " law " + law_type,
				 [&definitions](const std::string& value) { return definitions.law_values.contains(value); },
				 unknown_laws);
		}
		for (const auto& reform: country.reforms)
		{
			recordIfUnknown(reform,
				 "country " + tag,
				 [&definitions](const std::string& value) { return definitions.government_reforms.contains(value); },
				 unknown_reforms);
		}
		for (const auto& privilege: country.privileges)
		{
			recordIfUnknown(privilege,
				 "country " + tag,
				 [&definitions](const std::string& value) { return definitions.estate_privileges.contains(value); },
				 unknown_privileges);
		}

		if (!country.capital_location.empty())
		{
			const auto capital_it = world.locations.find(country.capital_location);
			if (capital_it == world.locations.end())
			{
				diagnostics.warning("VALIDATION_MISSING_CAPITAL",
					 "Country " + tag + " references missing capital location " + country.capital_location + '.');
			}
			else if (capital_it->second.owner_tag != tag)
			{
				diagnostics.warning("VALIDATION_FOREIGN_CAPITAL",
					 "Country " + tag + " capital " + country.capital_location + " is owned by " + capital_it->second.owner_tag + '.');
			}
		}

		const auto validateCharacterReference = [&](const std::string& character_key, const std::string& role) {
			if (character_key.empty())
			{
				return;
			}

			const auto character_it = world.characters.find(character_key);
			if (character_it == world.characters.end())
			{
				diagnostics.warning("VALIDATION_MISSING_COUNTRY_CHARACTER",
					 "Country " + tag + " references missing " + role + ' ' + character_key + '.');
				return;
			}

			if (character_it->second.tag != tag)
			{
				diagnostics.warning("VALIDATION_FOREIGN_COUNTRY_CHARACTER",
					 "Country " + tag + ' ' + role + ' ' + character_key + " belongs to " + character_it->second.tag + '.');
			}
			if (isDateAfter(character_it->second.birth_date, world.date))
			{
				diagnostics.warning("VALIDATION_UNBORN_COUNTRY_CHARACTER",
					 "Country " + tag + ' ' + role + ' ' + character_key + " is born after start date.");
			}
			if (!character_it->second.death_date.empty() && !isDateAfter(character_it->second.death_date, world.date))
			{
				diagnostics.warning("VALIDATION_DEAD_COUNTRY_CHARACTER",
					 "Country " + tag + ' ' + role + ' ' + character_key + " is already dead at start.");
			}
		};

		validateCharacterReference(country.ruler_character_key, "ruler");
		validateCharacterReference(country.consort_character_key, "consort");
		validateCharacterReference(country.heir_character_key, "heir");
	}

	for (const auto& [key, character]: world.characters)
	{
		recordIfUnknown(character.culture,
			 "character " + key,
			 [&definitions](const std::string& value) { return definitions.cultures.contains(value); },
			 unknown_cultures);
		recordIfUnknown(character.religion,
			 "character " + key,
			 [&definitions](const std::string& value) { return definitions.religions.contains(value); },
			 unknown_religions);
	}

	for (const auto& [key, location]: world.locations)
	{
		recordIfUnknown(location.culture,
			 "location " + key,
			 [&definitions](const std::string& value) { return definitions.cultures.contains(value); },
			 unknown_cultures);
		recordIfUnknown(location.religion,
			 "location " + key,
			 [&definitions](const std::string& value) { return definitions.religions.contains(value); },
			 unknown_religions);
		for (const auto& pop: location.pops)
		{
			recordIfUnknown(pop.culture,
				 "pop in " + key,
				 [&definitions](const std::string& value) { return definitions.cultures.contains(value); },
				 unknown_cultures);
			recordIfUnknown(pop.religion,
				 "pop in " + key,
				 [&definitions](const std::string& value) { return definitions.religions.contains(value); },
				 unknown_religions);
			recordIfUnknown(pop.type,
				 "pop in " + key,
				 [&definitions](const std::string& value) { return definitions.pop_types.contains(value); },
				 unknown_pop_types);
		}
	}

	for (const auto& relation: world.subject_relations)
	{
		recordIfUnknown(relation.subject_type,
			 "dependency " + relation.liege_tag + "->" + relation.subject_tag,
			 [&definitions](const std::string& value) { return definitions.subject_types.contains(value); },
			 unknown_subject_types);
		recordIfUnknown(relation.subject_military_stance,
			 "dependency " + relation.liege_tag + "->" + relation.subject_tag,
			 [&definitions](const std::string& value) { return definitions.subject_military_stances.contains(value); },
			 unknown_subject_stances);
	}

	for (const auto& relation: world.scripted_relations)
	{
		recordIfUnknown(relation.type,
			 "relation " + relation.first_tag + "->" + relation.second_tag,
			 [&definitions](const std::string& value) { return definitions.scripted_relations.contains(value); },
			 unknown_scripted_relations);
	}

	for (const auto& building: world.buildings)
	{
		recordIfUnknown(building.type,
			 "building in " + building.location,
			 [&definitions](const std::string& value) { return definitions.building_types.contains(value); },
				 unknown_buildings);
		const auto location_it = world.locations.find(building.location);
		if (location_it == world.locations.end())
		{
			diagnostics.warning("VALIDATION_MISSING_BUILDING_LOCATION",
				 "Building " + building.type + " references missing location " + building.location + '.');
		}
		else if (!building.tag.empty() && location_it->second.owner_tag != building.tag)
		{
			diagnostics.warning("VALIDATION_FOREIGN_BUILDING_OWNER",
				 "Building " + building.type + " at " + building.location + " is tagged to " + building.tag + " but owned by " +
						 location_it->second.owner_tag + '.');
		}
	}

	for (const auto& force: world.start_forces)
	{
		for (const auto& unit: force.units)
		{
			recordIfUnknown(unit.type,
				 "start force " + force.key,
				 [&definitions](const std::string& value) { return definitions.unit_types.contains(value); },
				 unknown_units);
		}
	}

	flushUnknowns("VALIDATION_UNKNOWN_CULTURE", "culture", unknown_cultures, diagnostics);
	flushUnknowns("VALIDATION_UNKNOWN_RELIGION", "religion", unknown_religions, diagnostics);
	flushUnknowns("VALIDATION_UNKNOWN_GOVERNMENT", "government type", unknown_governments, diagnostics);
	flushUnknowns("VALIDATION_UNKNOWN_COUNTRY_RANK", "country rank", unknown_country_ranks, diagnostics);
	flushUnknowns("VALIDATION_UNKNOWN_POP_TYPE", "pop type", unknown_pop_types, diagnostics);
	flushUnknowns("VALIDATION_UNKNOWN_HEIR_SELECTION", "heir selection", unknown_heir_selections, diagnostics);
	flushUnknowns("VALIDATION_UNKNOWN_LAW", "law value", unknown_laws, diagnostics);
	flushUnknowns("VALIDATION_UNKNOWN_GOVERNMENT_REFORM", "government reform", unknown_reforms, diagnostics);
	flushUnknowns("VALIDATION_UNKNOWN_PRIVILEGE", "estate privilege", unknown_privileges, diagnostics);
	flushUnknowns("VALIDATION_UNKNOWN_SUBJECT_TYPE", "subject type", unknown_subject_types, diagnostics);
	flushUnknowns("VALIDATION_UNKNOWN_BUILDING", "building type", unknown_buildings, diagnostics);
	flushUnknowns("VALIDATION_UNKNOWN_UNIT", "unit type", unknown_units, diagnostics);
	flushUnknowns("VALIDATION_UNKNOWN_SCRIPTED_RELATION", "scripted relation", unknown_scripted_relations, diagnostics);
	flushUnknowns("VALIDATION_UNKNOWN_SUBJECT_STANCE", "subject military stance", unknown_subject_stances, diagnostics);
}

}  // namespace ck3eu5::eu5
