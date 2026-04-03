#include "eu5/world_sanitizer.h"

#include "common/string_utils.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ck3eu5::eu5 {
namespace {

constexpr std::string_view kDefaultCulture = "english";
constexpr std::string_view kDefaultReligion = "catholic";

std::string stripTrailingSuffix(std::string value, const std::string_view suffix)
{
	if (value.size() > suffix.size() && value.ends_with(suffix))
	{
		value.resize(value.size() - suffix.size());
	}
	return value;
}

std::string normalizeLookupKey(std::string_view value)
{
	auto normalized = common::sanitizeIdentifier(value);
	normalized = stripTrailingSuffix(std::move(normalized), "_culture");
	normalized = stripTrailingSuffix(std::move(normalized), "_religion");
	normalized = stripTrailingSuffix(std::move(normalized), "_faith");
	normalized = stripTrailingSuffix(std::move(normalized), "_religious");
	normalized = stripTrailingSuffix(std::move(normalized), "_paganism");
	normalized = stripTrailingSuffix(std::move(normalized), "_pagan");
	normalized = stripTrailingSuffix(std::move(normalized), "_ism");
	return normalized;
}

std::vector<std::string> tokenizeLookupKey(const std::string& value)
{
	std::vector<std::string> tokens;
	for (auto& token: common::split(value, '_'))
	{
		token = stripTrailingSuffix(std::move(token), "ism");
		if (!token.empty() && token != "culture" && token != "religion" && token != "faith" && token != "pagan" &&
			token != "paganism")
		{
			tokens.push_back(token);
		}
	}
	return tokens;
}

bool containsToken(const std::vector<std::string>& tokens, const std::string& token)
{
	return std::find(tokens.begin(), tokens.end(), token) != tokens.end();
}

std::string fallbackValue(const std::set<std::string>& values, const std::string_view preferred)
{
	if (values.contains(std::string(preferred)))
	{
		return std::string(preferred);
	}
	return values.empty() ? std::string{} : *values.begin();
}

struct DefinitionResolver
{
	const std::set<std::string>& definitions;
	std::string default_value;
	std::unordered_map<std::string, std::string> aliases;
	mutable std::unordered_map<std::string, std::string> cache;

	std::string resolve(const std::string& value, const std::string& fallback = {}) const
	{
		const auto normalized = normalizeLookupKey(value);
		if (normalized.empty())
		{
			return fallback;
		}

		if (const auto cache_it = cache.find(normalized); cache_it != cache.end())
		{
			return cache_it->second.empty() ? fallback : cache_it->second;
		}

		std::string resolved;
		const auto tryCandidate = [&](const std::string& candidate) {
			if (!candidate.empty() && definitions.contains(candidate))
			{
				resolved = candidate;
				return true;
			}
			return false;
		};

		tryCandidate(value);
		tryCandidate(common::sanitizeIdentifier(value));
		tryCandidate(normalized);
		tryCandidate(normalized + "_culture");
		tryCandidate(normalized + "_religion");
		tryCandidate(normalized + "_faith");
		tryCandidate(normalized + "ism");

		if (resolved.empty())
		{
			if (const auto alias_it = aliases.find(normalized); alias_it != aliases.end())
			{
				tryCandidate(alias_it->second);
			}
		}

		if (resolved.empty())
		{
			const auto input_tokens = tokenizeLookupKey(normalized);
			int best_score = 0;
			std::string best_candidate;
			for (const auto& candidate: definitions)
			{
				const auto candidate_normalized = normalizeLookupKey(candidate);
				const auto candidate_tokens = tokenizeLookupKey(candidate_normalized);
				int score = 0;

				if (candidate_normalized == normalized)
				{
					score += 100;
				}
				if (candidate_normalized.find(normalized) != std::string::npos ||
					normalized.find(candidate_normalized) != std::string::npos)
				{
					score += 40;
				}

				for (const auto& token: input_tokens)
				{
					if (containsToken(candidate_tokens, token))
					{
						score += 12;
					}
					else if (candidate_normalized.find(token) != std::string::npos)
					{
						score += 6;
					}
				}

				if (score > best_score || (score == best_score && (best_candidate.empty() || candidate < best_candidate)))
				{
					best_score = score;
					best_candidate = candidate;
				}
			}

			if (best_score >= 12)
			{
				resolved = best_candidate;
			}
		}

		cache[normalized] = resolved;
		if (!resolved.empty())
		{
			return resolved;
		}
		return fallback;
	}
};

DefinitionResolver buildCultureResolver(const InstalledDefinitions& definitions)
{
	DefinitionResolver resolver{definitions.cultures, fallbackValue(definitions.cultures, kDefaultCulture), {}, {}};
	resolver.aliases = {
		 {"anglo_saxon", "english"},
		 {"franconian", "east_franconian"},
		 {"polish", "greater_polish"},
		 {"russian", "muscovite"},
		 {"persian", "farsi_culture"},
		 {"turkish", "turkish_culture"},
		 {"bedouin", "bedouin_culture"},
		 {"levantine", "levantine_culture"},
		 {"greek", "greek_culture"},
		 {"armenian", "armenian_culture"},
		 {"georgian", "georgian_culture"}};
	return resolver;
}

DefinitionResolver buildReligionResolver(const InstalledDefinitions& definitions)
{
	DefinitionResolver resolver{definitions.religions, fallbackValue(definitions.religions, kDefaultReligion), {}, {}};
	resolver.aliases = {
		 {"ashari", "sunni"},
		 {"maturidi", "sunni"},
		 {"ismaili", "shia"},
		 {"nestorian", "nestorianism"},
		 {"norse_pagan", "norse"},
		 {"tengri_pagan", "tengri"},
		 {"miaphysite_faith", "miaphysite"},
		 {"apostolic", "miaphysite"},
		 {"generic_faith", std::string(kDefaultReligion)}};
	return resolver;
}

bool isImpassableLocation(const LocationDefinition* definition)
{
	if (!definition)
	{
		return true;
	}

	const auto topography = common::toLower(definition->topography);
	return topography.find("wasteland") != std::string::npos || topography.find("coastal_ocean") != std::string::npos ||
			 topography.find("impassable") != std::string::npos || topography.find("inland_sea") != std::string::npos ||
			 topography == "ocean" || topography.find("narrows") != std::string::npos || topography.find("atoll") != std::string::npos;
}

bool isConnectorLocation(const LocationDefinition* definition)
{
	if (!definition)
	{
		return true;
	}

	return definition->key.starts_with("connector_") || common::toLower(definition->display_name).starts_with("connector");
}

bool isArmySpawnCandidate(const World& world,
	 const WorldFramework& framework,
	 const std::string& tag,
	 const std::string& location_key)
{
	const auto location_it = world.locations.find(location_key);
	if (location_it == world.locations.end() || location_it->second.owner_tag != tag)
	{
		return false;
	}

	const auto* definition = framework.getLocation(location_key);
	if (!definition || isImpassableLocation(definition) || isConnectorLocation(definition))
	{
		return false;
	}

	const bool inherently_urban = definition->default_rank == "city" || definition->default_rank == "town" || !definition->town_setup.empty();
	return inherently_urban;
}

bool isNavySpawnCandidate(const World& world,
	 const WorldFramework& framework,
	 const std::string& tag,
	 const std::string& location_key)
{
	const auto* definition = framework.getLocation(location_key);
	return isArmySpawnCandidate(world, framework, tag, location_key) && definition && definition->has_port;
}

double locationScore(const World& world, const WorldFramework& framework, const std::string& location_key, const std::string& capital_key)
{
	const auto location_it = world.locations.find(location_key);
	if (location_it == world.locations.end())
	{
		return -1000000.0;
	}

	double score = location_it->second.development;
	if (location_key == capital_key)
	{
		score += 500.0;
	}

	if (location_it->second.rank == "city")
	{
		score += 120.0;
	}
	else if (location_it->second.rank == "town")
	{
		score += 80.0;
	}

	if (const auto* definition = framework.getLocation(location_key); definition)
	{
		if (definition->has_port)
		{
			score += 15.0;
		}
		if (!definition->town_setup.empty())
		{
			score += 5.0;
		}
	}

	return score;
}

template <typename Predicate>
std::string chooseBestOwnedLocation(const World& world,
	 const WorldFramework& framework,
	 const Country& country,
	 const std::vector<std::string>& preferred_locations,
	 const Predicate& predicate)
{
	std::vector<std::string> candidates;
	std::unordered_set<std::string> seen;
	const auto addCandidate = [&](const std::string& location_key) {
		if (!location_key.empty() && seen.insert(location_key).second)
		{
			candidates.push_back(location_key);
		}
	};

	for (const auto& preferred: preferred_locations)
	{
		addCandidate(preferred);
	}
	for (const auto& location_key: country.owned_core_locations)
	{
		addCandidate(location_key);
	}

	double best_score = -1000000.0;
	std::string best_location;
	for (const auto& location_key: candidates)
	{
		if (!predicate(world, framework, country.tag, location_key))
		{
			continue;
		}
		const auto score = locationScore(world, framework, location_key, country.capital_location);
		if (score > best_score)
		{
			best_score = score;
			best_location = location_key;
		}
	}
	return best_location;
}

bool isPassableOwnedLocation(const World& world,
	 const WorldFramework& framework,
	 const std::string& tag,
	 const std::string& location_key)
{
	const auto location_it = world.locations.find(location_key);
	if (location_it == world.locations.end() || location_it->second.owner_tag != tag)
	{
		return false;
	}

	const auto* definition = framework.getLocation(location_key);
	return definition && !isImpassableLocation(definition) && !isConnectorLocation(definition);
}

bool isAnyOwnedLocation(const World& world,
	 const WorldFramework&,
	 const std::string& tag,
	 const std::string& location_key)
{
	const auto location_it = world.locations.find(location_key);
	return location_it != world.locations.end() && location_it->second.owner_tag == tag;
}

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

	const auto parts = common::split(text, '.');
	if (!parts.empty())
	{
		parsed.year = std::stoi(parts[0]);
	}
	if (parts.size() > 1)
	{
		parsed.month = std::stoi(parts[1]);
	}
	if (parts.size() > 2)
	{
		parsed.day = std::stoi(parts[2]);
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

bool isDateOnOrBefore(const std::string& lhs, const std::string& rhs)
{
	return !lhs.empty() && !isDateAfter(lhs, rhs);
}

bool isViableCountryCharacter(const World& world,
	 const std::string& start_date,
	 const std::string& tag,
	 const std::string& character_key)
{
	if (character_key.empty())
	{
		return false;
	}

	const auto character_it = world.characters.find(character_key);
	if (character_it == world.characters.end())
	{
		return false;
	}

	const auto& character = character_it->second;
	if (character.tag != tag)
	{
		return false;
	}
	if (isDateAfter(character.birth_date, start_date))
	{
		return false;
	}
	if (!character.death_date.empty() && isDateOnOrBefore(character.death_date, start_date))
	{
		return false;
	}
	return true;
}

std::string chooseFallbackRuler(const World& world, const std::string& start_date, const std::string& tag)
{
	for (const auto& [character_key, character]: world.characters)
	{
		if (character.tag == tag && isViableCountryCharacter(world, start_date, tag, character_key))
		{
			return character_key;
		}
	}
	return {};
}

template <typename Resolver>
void addResolvedWeight(std::map<std::string, double>& weights,
	 const Resolver& resolver,
	 const std::string& raw_value,
	 const double weight)
{
	if (weight <= 0.0)
	{
		return;
	}

	const auto resolved = resolver.resolve(raw_value, {});
	if (!resolved.empty())
	{
		weights[resolved] += weight;
	}
}

template <typename Resolver>
std::string pickBestResolvedValue(const Resolver& resolver, const std::map<std::string, double>& weights)
{
	if (weights.empty())
	{
		return resolver.default_value;
	}

	auto best_it = std::max_element(weights.begin(), weights.end(), [](const auto& lhs, const auto& rhs) {
		return lhs.second < rhs.second;
	});
	return best_it == weights.end() ? resolver.default_value : best_it->first;
}

void rebuildOwnershipAndPrune(World& world, diagnostics::DiagnosticsReport& diagnostics)
{
	for (auto& [tag, country]: world.countries)
	{
		country.owned_core_locations.clear();
		country.discovered_regions.clear();
	}

	for (const auto& [location_key, location]: world.locations)
	{
		if (const auto country_it = world.countries.find(location.owner_tag); country_it != world.countries.end())
		{
			country_it->second.owned_core_locations.insert(location_key);
			if (!location.region.empty())
			{
				country_it->second.discovered_regions.insert(location.region);
			}
		}
	}

	std::unordered_set<std::string> kept_tags;
	for (const auto& [tag, country]: world.countries)
	{
		if (!country.owned_core_locations.empty())
		{
			kept_tags.insert(tag);
		}
	}

	const auto original_country_count = world.countries.size();
	for (auto it = world.countries.begin(); it != world.countries.end();)
	{
		if (it->second.owned_core_locations.empty())
		{
			it = world.countries.erase(it);
		}
		else
		{
			++it;
		}
	}

	auto tagExists = [&kept_tags](const std::string& tag) {
		return !tag.empty() && kept_tags.contains(tag);
	};

	for (auto it = world.characters.begin(); it != world.characters.end();)
	{
		if (!it->second.tag.empty() && !tagExists(it->second.tag))
		{
			it = world.characters.erase(it);
		}
		else
		{
			++it;
		}
	}

	world.subject_relations.erase(std::remove_if(world.subject_relations.begin(),
								 world.subject_relations.end(),
								 [&tagExists](const SubjectRelation& relation) {
									 return !tagExists(relation.liege_tag) || !tagExists(relation.subject_tag) ||
												relation.liege_tag == relation.subject_tag;
								 }),
			 world.subject_relations.end());
	world.scripted_relations.erase(std::remove_if(world.scripted_relations.begin(),
								 world.scripted_relations.end(),
								 [&tagExists](const ScriptedRelation& relation) {
									 return !tagExists(relation.first_tag) || !tagExists(relation.second_tag) ||
												relation.first_tag == relation.second_tag;
								 }),
			 world.scripted_relations.end());
	world.opinions.erase(std::remove_if(world.opinions.begin(),
								 world.opinions.end(),
								 [&tagExists](const OpinionRelation& relation) {
									 return !tagExists(relation.first_tag) || !tagExists(relation.second_tag) ||
												relation.first_tag == relation.second_tag;
								 }),
			 world.opinions.end());
	world.rivals.erase(std::remove_if(world.rivals.begin(),
								 world.rivals.end(),
								 [&tagExists](const RivalRelation& relation) {
									 return !tagExists(relation.first_tag) || !tagExists(relation.second_tag) ||
												relation.first_tag == relation.second_tag;
								 }),
			 world.rivals.end());
	world.markets.erase(std::remove_if(world.markets.begin(),
								 world.markets.end(),
								 [&world, &tagExists](const MarketCenter& market) {
									 return !tagExists(market.owner_tag) || !world.locations.contains(market.location) ||
												world.locations.at(market.location).owner_tag != market.owner_tag;
								 }),
			 world.markets.end());

	for (auto& war: world.wars)
	{
		war.attackers.erase(std::remove_if(war.attackers.begin(),
									 war.attackers.end(),
									 [&tagExists](const WarParticipant& participant) { return !tagExists(participant.tag); }),
				war.attackers.end());
		war.defenders.erase(std::remove_if(war.defenders.begin(),
									 war.defenders.end(),
									 [&tagExists](const WarParticipant& participant) { return !tagExists(participant.tag); }),
				war.defenders.end());
		if (!war.attackers.empty())
		{
			war.first_tag = war.attackers.front().tag;
		}
		if (!war.defenders.empty())
		{
			war.second_tag = war.defenders.front().tag;
		}
	}
	world.wars.erase(std::remove_if(world.wars.begin(),
							 world.wars.end(),
							 [&tagExists](const War& war) {
								 return !tagExists(war.first_tag) || !tagExists(war.second_tag) || war.attackers.empty() || war.defenders.empty();
							 }),
			 world.wars.end());

	if (world.countries.size() != original_country_count)
	{
		diagnostics.warning("SANITIZER_COUNTRIES",
			 "Pruned " + std::to_string(original_country_count - world.countries.size()) +
					 " countries with no surviving owned locations after ownership reconciliation.");
	}
}

void pruneSingletonRegionOutliers(World& world, diagnostics::DiagnosticsReport& diagnostics)
{
	size_t removed_locations = 0;
	for (const auto& [tag, country]: world.countries)
	{
		if (country.owned_core_locations.size() < 10)
		{
			continue;
		}

		std::map<std::string, size_t> region_counts;
		for (const auto& location_key: country.owned_core_locations)
		{
			if (const auto location_it = world.locations.find(location_key); location_it != world.locations.end() &&
				!location_it->second.region.empty())
			{
				region_counts[location_it->second.region]++;
			}
		}

		if (region_counts.size() < 2)
		{
			continue;
		}

		const auto dominant_region_it = std::max_element(region_counts.begin(),
			 region_counts.end(),
			 [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });
		if (dominant_region_it == region_counts.end())
		{
			continue;
		}

		const auto dominant_count = dominant_region_it->second;
		const auto total_count = country.owned_core_locations.size();
		if (dominant_count * 100 < total_count * 90)
		{
			continue;
		}

		for (const auto& location_key: country.owned_core_locations)
		{
			const auto location_it = world.locations.find(location_key);
			if (location_it == world.locations.end() || location_it->second.region.empty() ||
				location_it->second.region == dominant_region_it->first)
			{
				continue;
			}

			const auto region_it = region_counts.find(location_it->second.region);
			if (region_it != region_counts.end() && region_it->second == 1)
			{
				world.locations.erase(location_it);
				++removed_locations;
			}
		}
	}

	if (removed_locations > 0)
	{
		rebuildOwnershipAndPrune(world, diagnostics);
		diagnostics.warning("SANITIZER_REGION_OUTLIERS",
			 "Removed " + std::to_string(removed_locations) +
					 " singleton cross-region ownership outliers from generated countries.");
	}
}

void applyCountrySlice(World& world,
	 const config::Configuration& configuration,
	 diagnostics::DiagnosticsReport& diagnostics)
{
	if (configuration.validation_country_limit <= 0)
	{
		return;
	}

	struct CountryOrdering
	{
		std::string tag;
		size_t owned_locations = 0;
		double total_development = 0.0;
	};

	std::vector<CountryOrdering> ordering;
	ordering.reserve(world.countries.size());
	for (const auto& [tag, country]: world.countries)
	{
		double total_development = 0.0;
		for (const auto& location_key: country.owned_core_locations)
		{
			if (const auto location_it = world.locations.find(location_key); location_it != world.locations.end())
			{
				total_development += location_it->second.development;
			}
		}
		ordering.push_back({tag, country.owned_core_locations.size(), total_development});
	}

	std::sort(ordering.begin(), ordering.end(), [](const CountryOrdering& lhs, const CountryOrdering& rhs) {
		if (lhs.owned_locations != rhs.owned_locations)
		{
			return lhs.owned_locations > rhs.owned_locations;
		}
		if (std::abs(lhs.total_development - rhs.total_development) > 0.001)
		{
			return lhs.total_development > rhs.total_development;
		}
		return lhs.tag < rhs.tag;
	});

	const auto start_index = static_cast<size_t>(std::min(configuration.validation_country_offset, static_cast<int>(ordering.size())));
	const auto end_index = std::min(ordering.size(), start_index + static_cast<size_t>(configuration.validation_country_limit));
	std::unordered_set<std::string> kept_tags;
	for (auto index = start_index; index < end_index; ++index)
	{
		kept_tags.insert(ordering[index].tag);
	}

	if (kept_tags.empty())
	{
		world.countries.clear();
		world.locations.clear();
		world.characters.clear();
		world.subject_relations.clear();
		world.scripted_relations.clear();
		world.opinions.clear();
		world.rivals.clear();
		world.markets.clear();
		world.buildings.clear();
		world.force_plans.clear();
		world.start_forces.clear();
		world.wars.clear();
		world.roads.clear();
		diagnostics.warning("SANITIZER_COUNTRY_SLICE", "Validation country slice removed the entire generated world.");
		return;
	}

	for (auto it = world.countries.begin(); it != world.countries.end();)
	{
		if (!kept_tags.contains(it->first))
		{
			it = world.countries.erase(it);
		}
		else
		{
			++it;
		}
	}

	for (auto it = world.locations.begin(); it != world.locations.end();)
	{
		if (!kept_tags.contains(it->second.owner_tag))
		{
			it = world.locations.erase(it);
		}
		else
		{
			++it;
		}
	}

	rebuildOwnershipAndPrune(world, diagnostics);

	diagnostics.warning("SANITIZER_COUNTRY_SLICE",
		 "Reduced generated world to " + std::to_string(world.countries.size()) + " countries using validation_country_offset=" +
				 std::to_string(configuration.validation_country_offset) + " and validation_country_limit=" +
				 std::to_string(configuration.validation_country_limit) + ".");
}

}  // namespace

void WorldSanitizer::sanitize(World& world,
	 const WorldFramework& framework,
	 const config::Configuration& configuration,
	 const InstalledDefinitions& definitions,
	 diagnostics::DiagnosticsReport& diagnostics) const
{
	size_t removed_locations = 0;
	for (auto it = world.locations.begin(); it != world.locations.end();)
	{
		const auto* definition = framework.getLocation(it->first);
		if (!definition || isImpassableLocation(definition) || isConnectorLocation(definition))
		{
			it = world.locations.erase(it);
			++removed_locations;
		}
		else
		{
			++it;
		}
	}

	rebuildOwnershipAndPrune(world, diagnostics);
	pruneSingletonRegionOutliers(world, diagnostics);
	applyCountrySlice(world, configuration, diagnostics);

	auto culture_resolver = buildCultureResolver(definitions);
	auto religion_resolver = buildReligionResolver(definitions);

	std::map<std::string, std::string> country_fallback_cultures;
	std::map<std::string, std::string> country_fallback_religions;

	for (const auto& [tag, country]: world.countries)
	{
		std::map<std::string, double> culture_weights;
		std::map<std::string, double> religion_weights;
		addResolvedWeight(culture_weights, culture_resolver, country.primary_culture, 50.0);
		addResolvedWeight(religion_weights, religion_resolver, country.primary_religion, 50.0);

		for (const auto& location_key: country.owned_core_locations)
		{
			if (const auto location_it = world.locations.find(location_key); location_it != world.locations.end())
			{
				addResolvedWeight(culture_weights, culture_resolver, location_it->second.culture, std::max(1.0, location_it->second.development));
				addResolvedWeight(religion_weights, religion_resolver, location_it->second.religion, std::max(1.0, location_it->second.development));
				for (const auto& pop: location_it->second.pops)
				{
					addResolvedWeight(culture_weights, culture_resolver, pop.culture, std::max(0.001, pop.size) * 5.0);
					addResolvedWeight(religion_weights, religion_resolver, pop.religion, std::max(0.001, pop.size) * 5.0);
				}
			}
		}

		for (const auto& [character_key, character]: world.characters)
		{
			if (character.tag != tag)
			{
				continue;
			}
			addResolvedWeight(culture_weights, culture_resolver, character.culture, 3.0);
			addResolvedWeight(religion_weights, religion_resolver, character.religion, 3.0);
		}

		country_fallback_cultures[tag] = pickBestResolvedValue(culture_resolver, culture_weights);
		country_fallback_religions[tag] = pickBestResolvedValue(religion_resolver, religion_weights);
	}

	size_t culture_rewrites = 0;
	size_t religion_rewrites = 0;

	for (auto& [tag, country]: world.countries)
	{
		const auto culture_fallback = country_fallback_cultures.contains(tag) ? country_fallback_cultures.at(tag) : culture_resolver.default_value;
		const auto religion_fallback =
				religion_resolver.default_value.empty() ? fallbackValue(definitions.religions, kDefaultReligion) :
				(country_fallback_religions.contains(tag) ? country_fallback_religions.at(tag) : religion_resolver.default_value);

		const auto original_primary_culture = country.primary_culture;
		const auto original_primary_religion = country.primary_religion;
		country.primary_culture = culture_resolver.resolve(country.primary_culture, culture_fallback);
		country.primary_religion = religion_resolver.resolve(country.primary_religion, religion_fallback);
		culture_rewrites += static_cast<size_t>(country.primary_culture != original_primary_culture);
		religion_rewrites += static_cast<size_t>(country.primary_religion != original_primary_religion);

		std::set<std::string> accepted_cultures;
		for (const auto& culture: country.accepted_cultures)
		{
			const auto resolved = culture_resolver.resolve(culture, country.primary_culture);
			if (resolved != country.primary_culture)
			{
				accepted_cultures.insert(resolved);
			}
		}
		country.accepted_cultures = std::move(accepted_cultures);

		std::set<std::string> tolerated_cultures;
		for (const auto& culture: country.tolerated_cultures)
		{
			const auto resolved = culture_resolver.resolve(culture, country.primary_culture);
			if (resolved != country.primary_culture && !country.accepted_cultures.contains(resolved))
			{
				tolerated_cultures.insert(resolved);
			}
		}
		country.tolerated_cultures = std::move(tolerated_cultures);
	}

	for (auto& [character_key, character]: world.characters)
	{
		const auto culture_fallback = country_fallback_cultures.contains(character.tag) ? country_fallback_cultures.at(character.tag) : culture_resolver.default_value;
		const auto religion_fallback =
				country_fallback_religions.contains(character.tag) ? country_fallback_religions.at(character.tag) : religion_resolver.default_value;
		const auto original_culture = character.culture;
		const auto original_religion = character.religion;
		character.culture = culture_resolver.resolve(character.culture, culture_fallback);
		character.religion = religion_resolver.resolve(character.religion, religion_fallback);
		culture_rewrites += static_cast<size_t>(character.culture != original_culture);
		religion_rewrites += static_cast<size_t>(character.religion != original_religion);
	}

	for (auto& [location_key, location]: world.locations)
	{
		const auto culture_fallback =
				country_fallback_cultures.contains(location.owner_tag) ? country_fallback_cultures.at(location.owner_tag) : culture_resolver.default_value;
		const auto religion_fallback =
				country_fallback_religions.contains(location.owner_tag) ? country_fallback_religions.at(location.owner_tag) : religion_resolver.default_value;
		const auto original_culture = location.culture;
		const auto original_religion = location.religion;
		location.culture = culture_resolver.resolve(location.culture, culture_fallback);
		location.religion = religion_resolver.resolve(location.religion, religion_fallback);
		culture_rewrites += static_cast<size_t>(location.culture != original_culture);
		religion_rewrites += static_cast<size_t>(location.religion != original_religion);

		for (auto& pop: location.pops)
		{
			const auto original_pop_culture = pop.culture;
			const auto original_pop_religion = pop.religion;
			pop.culture = culture_resolver.resolve(pop.culture, location.culture);
			pop.religion = religion_resolver.resolve(pop.religion, location.religion);
			culture_rewrites += static_cast<size_t>(pop.culture != original_pop_culture);
			religion_rewrites += static_cast<size_t>(pop.religion != original_pop_religion);
		}
	}

	for (auto& [tag, country]: world.countries)
	{
		std::map<std::string, double> culture_weights;
		std::map<std::string, double> religion_weights;
		double total_population = 0.0;
		for (const auto& location_key: country.owned_core_locations)
		{
			if (const auto location_it = world.locations.find(location_key); location_it != world.locations.end())
			{
				for (const auto& pop: location_it->second.pops)
				{
					total_population += pop.size;
					culture_weights[pop.culture] += pop.size;
					religion_weights[pop.religion] += pop.size;
				}
			}
		}

		country.primary_culture = pickBestResolvedValue(culture_resolver, culture_weights);
		country.primary_religion = pickBestResolvedValue(religion_resolver, religion_weights);
		country.accepted_cultures.clear();
		country.tolerated_cultures.clear();
		for (const auto& [culture, weight]: culture_weights)
		{
			if (culture == country.primary_culture || total_population <= 0.0)
			{
				continue;
			}

			const auto share = weight / total_population;
			if (share >= 0.25)
			{
				country.accepted_cultures.insert(culture);
			}
			else if (share >= 0.10)
			{
				country.tolerated_cultures.insert(culture);
			}
		}
	}

	for (auto& [tag, country]: world.countries)
	{
		const auto alignLeaderReligion = [&](const std::string& character_key) {
			if (character_key.empty())
			{
				return;
			}
			const auto character_it = world.characters.find(character_key);
			if (character_it == world.characters.end() || character_it->second.tag != tag || country.primary_religion.empty())
			{
				return;
			}
			if (character_it->second.religion != country.primary_religion)
			{
				character_it->second.religion = country.primary_religion;
				++religion_rewrites;
			}
		};

		alignLeaderReligion(country.ruler_character_key);
		alignLeaderReligion(country.consort_character_key);
		alignLeaderReligion(country.heir_character_key);
	}

	size_t relocated_capitals = 0;
	for (auto& [tag, country]: world.countries)
	{
		auto safe_capital = chooseBestOwnedLocation(world, framework, country, {country.capital_location}, isArmySpawnCandidate);
		if (safe_capital.empty())
		{
			safe_capital = chooseBestOwnedLocation(world, framework, country, {country.capital_location}, isPassableOwnedLocation);
		}
		if (safe_capital.empty())
		{
			safe_capital = chooseBestOwnedLocation(world, framework, country, {country.capital_location}, isAnyOwnedLocation);
		}
		if (!safe_capital.empty() && safe_capital != country.capital_location)
		{
			country.capital_location = safe_capital;
			++relocated_capitals;
		}
	}

	size_t cleared_character_refs = 0;
	for (auto& [tag, country]: world.countries)
	{
		if (!isViableCountryCharacter(world, world.date, tag, country.ruler_character_key))
		{
			const auto fallback_ruler = chooseFallbackRuler(world, world.date, tag);
			if (fallback_ruler != country.ruler_character_key)
			{
				country.ruler_character_key = fallback_ruler;
				++cleared_character_refs;
			}
		}

		const bool keep_dynastic_relatives =
				country.government_type == "monarchy" || country.government_type == "tribe" || country.government_type == "steppe_horde";

		if (!keep_dynastic_relatives || !isViableCountryCharacter(world, world.date, tag, country.consort_character_key) ||
			 country.consort_character_key == country.ruler_character_key)
		{
			if (!country.consort_character_key.empty())
			{
				country.consort_character_key.clear();
				++cleared_character_refs;
			}
		}
		if (!keep_dynastic_relatives || !isViableCountryCharacter(world, world.date, tag, country.heir_character_key) ||
			 country.heir_character_key == country.ruler_character_key)
		{
			if (!country.heir_character_key.empty())
			{
				country.heir_character_key.clear();
				++cleared_character_refs;
			}
		}
	}

	size_t filtered_buildings = 0;
	size_t retagged_buildings = 0;
	world.buildings.erase(std::remove_if(world.buildings.begin(),
								 world.buildings.end(),
								 [&](BuildingInstance& building) {
									 const auto* definition = framework.getLocation(building.location);
									 if (!definition || !world.locations.contains(building.location))
									 {
										 ++filtered_buildings;
										 return true;
									 }
									 const auto& location = world.locations.at(building.location);
									 if (location.owner_tag.empty())
									 {
										 ++filtered_buildings;
										 return true;
									 }
									 if (!building.tag.empty() && building.tag != location.owner_tag)
									 {
										 building.tag = location.owner_tag;
										 ++retagged_buildings;
									 }
									 if ((building.type == "dock" || building.type == "dry_dock" || building.type == "coastal_fort") &&
										 !definition->has_port)
									 {
										 ++filtered_buildings;
										 return true;
									 }
									 if (building.type == "local_markets")
									 {
										 const auto location_it = world.locations.find(building.location);
										 if (location_it != world.locations.end() && location_it->second.rank == "city")
										 {
											 ++filtered_buildings;
											 return true;
										 }
									 }
									 return false;
								 }),
			 world.buildings.end());

	size_t relocated_forces = 0;
	size_t dropped_forces = 0;
	std::vector<StartForce> sanitized_forces;
	sanitized_forces.reserve(world.start_forces.size());
	for (auto force: world.start_forces)
	{
		const auto country_it = world.countries.find(force.tag);
		if (country_it == world.countries.end())
		{
			++dropped_forces;
			continue;
		}

		std::vector<std::string> preferred_locations = {force.location, country_it->second.capital_location};
		for (const auto& market: world.markets)
		{
			if (market.owner_tag == force.tag)
			{
				preferred_locations.push_back(market.location);
			}
		}

		const auto safe_location =
				force.branch == "navy" ? chooseBestOwnedLocation(world, framework, country_it->second, preferred_locations, isNavySpawnCandidate) :
									  chooseBestOwnedLocation(world, framework, country_it->second, preferred_locations, isArmySpawnCandidate);
		if (safe_location.empty())
		{
			++dropped_forces;
			continue;
		}
		if (safe_location != force.location)
		{
			force.location = safe_location;
			++relocated_forces;
		}
		sanitized_forces.push_back(std::move(force));
	}
	world.start_forces = std::move(sanitized_forces);

	size_t updated_force_plans = 0;
	for (auto& plan: world.force_plans)
	{
		const auto country_it = world.countries.find(plan.tag);
		if (country_it == world.countries.end())
		{
			continue;
		}
		const auto safe_location =
				plan.branch == "navy" ? chooseBestOwnedLocation(world,
											 framework,
											 country_it->second,
											 {plan.home_location, country_it->second.capital_location},
											 isNavySpawnCandidate) :
									  chooseBestOwnedLocation(world,
											 framework,
											 country_it->second,
											 {plan.home_location, country_it->second.capital_location},
											 isArmySpawnCandidate);
		if (!safe_location.empty() && safe_location != plan.home_location)
		{
			plan.home_location = safe_location;
			++updated_force_plans;
		}
	}

	diagnostics.info("SANITIZER_IDENTITIES",
		 "Rewrote " + std::to_string(culture_rewrites) + " culture references and " + std::to_string(religion_rewrites) +
				 " religion references against installed EU5 definitions.");
	if (removed_locations > 0)
	{
		diagnostics.warning("SANITIZER_LOCATIONS",
			 "Removed " + std::to_string(removed_locations) + " impassable or connector locations from generated ownership.");
	}
	if (relocated_capitals > 0)
	{
		diagnostics.warning("SANITIZER_CAPITALS", "Moved " + std::to_string(relocated_capitals) + " country capitals off unsafe locations.");
	}
	if (cleared_character_refs > 0)
	{
		diagnostics.warning("SANITIZER_CHARACTERS",
			 "Cleared or repaired " + std::to_string(cleared_character_refs) + " invalid country character references.");
	}
	if (filtered_buildings > 0)
	{
		diagnostics.warning("SANITIZER_BUILDINGS", "Dropped " + std::to_string(filtered_buildings) + " invalid building placements.");
	}
	if (retagged_buildings > 0)
	{
		diagnostics.warning("SANITIZER_BUILDINGS",
			 "Retagged " + std::to_string(retagged_buildings) + " building ownership references to match final location owners.");
	}
	if (relocated_forces > 0 || dropped_forces > 0 || updated_force_plans > 0)
	{
		diagnostics.warning("SANITIZER_FORCES",
			 "Relocated " + std::to_string(relocated_forces) + " startup forces, dropped " + std::to_string(dropped_forces) +
					 " unsafe startup forces, and updated " + std::to_string(updated_force_plans) + " force plans.");
	}
}

}  // namespace ck3eu5::eu5
