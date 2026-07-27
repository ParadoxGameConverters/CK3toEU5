#include "BlockParsing.h"
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>

namespace
{
// The keys through which a vanilla country entry actually holds land. own_core belongs here:
// it means owned-but-occupied. our_cores_conquered_by_others does not - those are mere claims.
const std::regex ownershipBlock(R"((own_control_core|own_control_integrated|own_control_conquered|own_control_colony|own_core|add_pops_from_locations)\s*=\s*\{([^}]*)\})");
const std::regex capitalRef(R"(capital\s*=\s*(\w+))");
} // namespace

std::string output::slurpFile(const std::filesystem::path& file)
{
	std::ifstream input(file);
	if (!input.is_open())
		return {};
	std::stringstream buffer;
	buffer << input.rdbuf();
	return buffer.str();
}

std::vector<std::pair<std::string, std::string>> output::extractNamedBlocks(const std::string& text, size_t bodyStart)
{
	std::vector<std::pair<std::string, std::string>> blocks;
	size_t pos = bodyStart;
	std::string currentName;
	size_t nameStart = std::string::npos;
	const auto isWord = [](char c) {
		return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.' || c == '\'';
	};
	while (pos < text.size())
	{
		const char c = text[pos];
		if (c == '#')
		{
			while (pos < text.size() && text[pos] != '\n')
				++pos;
			continue;
		}
		if (c == '}')
			break; // end of the body itself.
		if (isWord(c))
		{
			nameStart = pos;
			while (pos < text.size() && isWord(text[pos]))
				++pos;
			currentName = text.substr(nameStart, pos - nameStart);
			continue;
		}
		if (c == '{')
		{
			int depth = 1;
			++pos;
			while (pos < text.size() && depth > 0)
			{
				if (text[pos] == '#')
				{
					while (pos < text.size() && text[pos] != '\n')
						++pos;
					continue;
				}
				if (text[pos] == '{')
					++depth;
				else if (text[pos] == '}')
					--depth;
				++pos;
			}
			if (nameStart != std::string::npos)
				blocks.emplace_back(currentName, text.substr(nameStart, pos - nameStart));
			nameStart = std::string::npos;
			continue;
		}
		++pos; // '=', whitespace, quotes...
	}
	return blocks;
}

size_t output::findBlockBody(const std::string& text, const std::string& name, size_t from)
{
	const std::regex header(name + R"(\s*=\s*\{)");
	std::smatch match;
	auto searchStart = text.cbegin() + static_cast<std::ptrdiff_t>(from);
	if (!std::regex_search(searchStart, text.cend(), match, header))
		return std::string::npos;
	return from + static_cast<size_t>(match.position(0) + match.length(0));
}

std::string output::extractBlockBody(const std::string& text, const std::string& name, size_t from)
{
	const auto bodyStart = findBlockBody(text, name, from);
	if (bodyStart == std::string::npos)
		return {};
	auto pos = bodyStart;
	int depth = 1;
	while (pos < text.size() && depth > 0)
	{
		if (text[pos] == '#')
		{
			while (pos < text.size() && text[pos] != '\n')
				++pos;
			continue;
		}
		if (text[pos] == '{')
			++depth;
		else if (text[pos] == '}' && --depth == 0)
			break;
		++pos;
	}
	return text.substr(bodyStart, pos - bodyStart);
}

bool output::touchesConvertedLand(const std::string& countryBlock,
	 const std::set<std::string>& convertedLocations,
	 const EU5::LocationDefinitions& definitions)
{
	std::vector<std::string> tokens;
	for (auto match = std::sregex_iterator(countryBlock.begin(), countryBlock.end(), ownershipBlock); match != std::sregex_iterator(); ++match)
	{
		std::stringstream body((*match)[2].str());
		std::string token;
		while (body >> token)
			tokens.push_back(token);
	}
	if (std::smatch capital; std::regex_search(countryBlock, capital, capitalRef))
		tokens.push_back(capital[1].str());
	for (const auto& token: tokens)
	{
		if (convertedLocations.contains(token))
			return true;
		// Only names that aren't locations in their own right stand for a group. EU5 names a province
		// after its main location, so treating "qingchi" as its province would drag in every neighbour.
		if (definitions.isValidLocation(token))
			continue;
		if (const auto group = definitions.getGroupLocations().find(token); group != definitions.getGroupLocations().end())
			for (const auto& location: group->second)
				if (convertedLocations.contains(location))
					return true;
	}
	return false;
}

std::optional<std::string> output::trimToSurvivingLand(const std::string& countryBlock,
	 const std::set<std::string>& convertedLocations,
	 const EU5::LocationDefinitions& definitions)
{
	if (std::smatch capital; std::regex_search(countryBlock, capital, capitalRef))
		if (convertedLocations.contains(capital[1].str()))
			return std::nullopt; // a country without its capital is not the same country

	// A token is a location, or an area/province/region standing in for many. Groups that lost part
	// of their land are spelled out location by location so the survivors stay claimed. A name that
	// is itself a location is always just that location, never the province EU5 named after it.
	const auto surviving = [&](const std::string& token) -> std::vector<std::string> {
		if (convertedLocations.contains(token))
			return {};
		if (definitions.isValidLocation(token))
			return {token};
		const auto group = definitions.getGroupLocations().find(token);
		if (group == definitions.getGroupLocations().end())
			return {token};
		std::vector<std::string> kept;
		auto lost = false;
		for (const auto& location: group->second)
		{
			if (convertedLocations.contains(location))
				lost = true;
			else
				kept.push_back(location);
		}
		return lost ? kept : std::vector{token};
	};

	std::string trimmed;
	size_t last = 0;
	auto ownsAnything = false;
	auto changed = false;
	for (auto match = std::sregex_iterator(countryBlock.begin(), countryBlock.end(), ownershipBlock); match != std::sregex_iterator(); ++match)
	{
		const auto& key = (*match)[1].str();
		std::stringstream body((*match)[2].str());
		std::string token;
		std::vector<std::string> kept;
		auto blockChanged = false;
		while (body >> token)
		{
			const auto survivors = surviving(token);
			if (survivors.size() != 1 || survivors.front() != token)
				blockChanged = true;
			kept.insert(kept.end(), survivors.begin(), survivors.end());
		}
		if (!kept.empty() && key != "add_pops_from_locations")
			ownsAnything = true;
		if (!blockChanged)
			continue;
		changed = true;
		std::string rebuilt = key + " = {";
		for (size_t index = 0; index < kept.size(); ++index)
			rebuilt += (index % 10 == 0 ? "\n\t\t\t\t" : " ") + kept[index];
		rebuilt += kept.empty() ? " }" : "\n\t\t\t}";
		trimmed += countryBlock.substr(last, static_cast<size_t>(match->position(0)) - last) + rebuilt;
		last = static_cast<size_t>(match->position(0) + match->length(0));
	}
	if (!ownsAnything)
		return std::nullopt;
	if (!changed)
		return countryBlock;
	trimmed += countryBlock.substr(last);
	return trimmed;
}
