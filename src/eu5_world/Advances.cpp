#include "Advances.h"
#include "CommonFunctions.h"
#include "Log.h"
#include "OSCompatibilityLayer.h"
#include "src/output/BlockParsing.h"
#include <algorithm>
#include <fstream>
#include <ranges>
#include <regex>
#include <sstream>

namespace
{
std::string readFile(const std::filesystem::path& file)
{
	std::ifstream input(file);
	if (!input.is_open())
		return {};
	std::stringstream buffer;
	buffer << input.rdbuf();
	return buffer.str();
}
} // namespace

void EU5::Advances::loadAdvances(const std::filesystem::path& advancesFolder)
{
	if (!std::filesystem::exists(advancesFolder))
	{
		Log(LogLevel::Warning) << "No EU5 advances at " << advancesFolder.string() << "; converted laws will not be checked against technology.";
		return;
	}
	for (const auto& file: commonItems::GetAllFilesInFolder(advancesFolder))
		if (file.extension() == ".txt")
			readAdvanceText(readFile(advancesFolder / file));
	resolveAll();
}

void EU5::Advances::loadAdvancesFromStream(std::istream& theStream)
{
	std::stringstream buffer;
	buffer << theStream.rdbuf();
	readAdvanceText(buffer.str());
	resolveAll();
}

void EU5::Advances::readAdvanceText(const std::string& text)
{
	static const std::regex levelLine(R"(\bstarting_technology_level\s*=\s*(\d+))");
	static const std::regex requiresLine(R"(\brequires\s*=\s*(\w+))");
	static const std::regex unlockLine(R"(\bunlock_law\s*=\s*(\w+))");
	for (const auto& [name, block]: output::extractNamedBlocks(text, 0))
	{
		Advance advance;
		if (std::smatch match; std::regex_search(block, match, levelLine))
			advance.declaredLevel = std::stoi(match[1].str());
		for (auto it = std::sregex_iterator(block.begin(), block.end(), requiresLine); it != std::sregex_iterator(); ++it)
			advance.prerequisites.insert((*it)[1].str());
		for (auto it = std::sregex_iterator(block.begin(), block.end(), unlockLine); it != std::sregex_iterator(); ++it)
			advance.unlockedLaws.insert((*it)[1].str());
		// An advance only one country, culture or faith can ever take is no general route to its law.
		const auto potential = output::extractBlockBody(block, "potential");
		advance.countrySpecific = !potential.empty() && potential.find_first_not_of(" \t\r\n") != std::string::npos;
		advances[name] = advance;
	}
}

int EU5::Advances::resolveLevel(const std::string& advance, std::set<std::string>& visiting) const
{
	const auto match = advances.find(advance);
	if (match == advances.end() || !visiting.insert(advance).second)
		return 0; // unknown prerequisite, or a cycle in the tree; assume it costs nothing
	auto level = std::max(0, match->second.declaredLevel);
	// An advance that declares no level of its own is reached as soon as everything below it is.
	for (const auto& prerequisite: match->second.prerequisites)
		level = std::max(level, resolveLevel(prerequisite, visiting));
	visiting.erase(advance);
	return level;
}

void EU5::Advances::resolveAll()
{
	for (const auto& name: advances | std::views::keys)
	{
		std::set<std::string> visiting;
		advanceLevels[name] = resolveLevel(name, visiting);
	}
	for (const auto& [name, advance]: advances)
	{
		if (advance.countrySpecific)
			continue;
		for (const auto& law: advance.unlockedLaws)
		{
			const auto level = advanceLevels.at(name);
			const auto existing = lawLevels.find(law);
			if (existing == lawLevels.end() || level < existing->second)
				lawLevels[law] = level;
		}
	}
}

void EU5::Advances::loadPrivileges(const std::filesystem::path& privilegesFolder)
{
	if (!std::filesystem::exists(privilegesFolder))
		return;
	for (const auto& file: commonItems::GetAllFilesInFolder(privilegesFolder))
		if (file.extension() == ".txt")
			readPrivilegeText(readFile(privilegesFolder / file));
}

void EU5::Advances::loadPrivilegesFromStream(std::istream& theStream)
{
	std::stringstream buffer;
	buffer << theStream.rdbuf();
	readPrivilegeText(buffer.str());
}

void EU5::Advances::readPrivilegeText(const std::string& text)
{
	static const std::regex advanceLine(R"(\bhas_advance\s*=\s*(\w+))");
	for (const auto& [name, block]: output::extractNamedBlocks(text, 0))
	{
		auto level = 0;
		for (auto it = std::sregex_iterator(block.begin(), block.end(), advanceLine); it != std::sregex_iterator(); ++it)
			if (const auto match = advanceLevels.find((*it)[1].str()); match != advanceLevels.end())
				level = std::max(level, match->second);
		if (level > 0)
			privilegeLevels[name] = level;
	}
}

int EU5::Advances::getLawTechLevel(const std::string& law) const
{
	const auto match = lawLevels.find(law);
	return match != lawLevels.end() ? match->second : 0;
}

int EU5::Advances::getPrivilegeTechLevel(const std::string& privilege) const
{
	const auto match = privilegeLevels.find(privilege);
	return match != privilegeLevels.end() ? match->second : 0;
}
