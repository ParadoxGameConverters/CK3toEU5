#include "LocationDefinitions.h"
#include "Log.h"
#include <fstream>
#include <sstream>

void EU5::LocationDefinitions::loadDefinitions(const std::filesystem::path& definitionsFile)
{
	std::ifstream theFile(definitionsFile);
	if (!theFile.is_open())
	{
		Log(LogLevel::Warning) << "Could not open " << definitionsFile.string() << " - location validation will be disabled!";
		return;
	}
	parseTokens(theFile);
	theFile.close();
}

void EU5::LocationDefinitions::loadDefinitions(std::istream& theStream)
{
	parseTokens(theStream);
}

void EU5::LocationDefinitions::parseTokens(std::istream& theStream)
{
	// Tokenize the stream. Any bare word not followed by '=' is a location; words followed by '=' name grouping blocks.
	std::vector<std::string> tokens;
	std::string line;
	while (std::getline(theStream, line))
	{
		if (const auto commentPos = line.find('#'); commentPos != std::string::npos)
			line = line.substr(0, commentPos);
		std::string current;
		for (const auto character: line)
		{
			if (character == '{' || character == '}' || character == '=')
			{
				if (!current.empty())
				{
					tokens.push_back(current);
					current.clear();
				}
				tokens.emplace_back(1, character);
			}
			else if (std::isspace(static_cast<unsigned char>(character)))
			{
				if (!current.empty())
				{
					tokens.push_back(current);
					current.clear();
				}
			}
			else
			{
				current += character;
			}
		}
		if (!current.empty())
			tokens.push_back(current);
	}

	// Walk the block hierarchy (continent > subcontinent > region > area > province > locations),
	// tracking each location's continent and every region under each continent.
	std::vector<std::string> blockStack;
	std::string pendingName;
	for (size_t i = 0; i < tokens.size(); ++i)
	{
		const auto& token = tokens[i];
		if (token == "=")
			continue;
		if (token == "{")
		{
			blockStack.push_back(pendingName);
			pendingName.clear();
			continue;
		}
		if (token == "}")
		{
			if (!blockStack.empty())
				blockStack.pop_back();
			continue;
		}
		if (i + 1 < tokens.size() && tokens[i + 1] == "=")
		{
			pendingName = token; // grouping block name.
			continue;
		}
		locations.insert(token);
		if (blockStack.empty())
			continue;
		locationContinents[token] = blockStack.front();
		for (const auto& block: blockStack)
			if (!block.empty())
				groupLocations[block].insert(token);
		for (auto block = blockStack.rbegin(); block != blockStack.rend(); ++block)
		{
			if (block->ends_with("_region"))
			{
				continentRegions[blockStack.front()].insert(*block);
				break;
			}
		}
	}
}

void EU5::LocationDefinitions::loadPorts(const std::filesystem::path& portsFile)
{
	// ports.csv: LandProvince;SeaZone;x;y; with a header line.
	std::ifstream theFile(portsFile);
	if (!theFile.is_open())
	{
		Log(LogLevel::Warning) << "Could not open " << portsFile.string() << " - coastal detection will be disabled!";
		return;
	}
	std::string line;
	std::getline(theFile, line); // header.
	while (std::getline(theFile, line))
	{
		const auto firstSplit = line.find(';');
		if (firstSplit == std::string::npos)
			continue;
		const auto secondSplit = line.find(';', firstSplit + 1);
		if (secondSplit == std::string::npos)
			continue;
		const auto location = line.substr(0, firstSplit);
		const auto seaZone = line.substr(firstSplit + 1, secondSplit - firstSplit - 1);
		if (!location.empty() && !seaZone.empty())
			portSeaZones[location] = seaZone;
	}
	theFile.close();
}

std::string EU5::LocationDefinitions::getPortSeaZone(const std::string& location) const
{
	if (const auto& match = portSeaZones.find(location); match != portSeaZones.end())
		return match->second;
	return {};
}

std::string EU5::LocationDefinitions::getContinentForLocation(const std::string& location) const
{
	if (const auto& match = locationContinents.find(location); match != locationContinents.end())
		return match->second;
	return {};
}
