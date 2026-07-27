#include "LawMapper.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"

namespace
{
class ValueLink: commonItems::parser
{
  public:
	explicit ValueLink(std::istream& theStream)
	{
		registerKeyword("ck3", [this](std::istream& stream) {
			ck3Law = commonItems::getString(stream);
		});
		registerKeyword("key", [this](std::istream& stream) {
			key = commonItems::getString(stream);
		});
		registerKeyword("offset", [this](std::istream& stream) {
			offset = commonItems::getInt(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string ck3Law;
	std::string key;
	int offset = 0;
};

// An ethos or tradition link. Unlike laws, these carry several key/shift pairs, since a cultural
// pillar rarely says just one thing about how a realm is run.
class ShiftLink: commonItems::parser
{
  public:
	explicit ShiftLink(std::istream& theStream)
	{
		registerKeyword("ck3", [this](std::istream& stream) {
			ck3Pillar = commonItems::getString(stream);
		});
		registerKeyword("key", [this](std::istream& stream) {
			pendingKeys.push_back(commonItems::getString(stream));
		});
		registerKeyword("shift", [this](std::istream& stream) {
			const auto shift = commonItems::getInt(stream);
			if (!pendingKeys.empty())
			{
				shifts[pendingKeys.back()] = shift;
				pendingKeys.pop_back();
			}
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string ck3Pillar;
	std::map<std::string, int> shifts;

  private:
	std::vector<std::string> pendingKeys; // a key waits here until its shift is read
};

class HeirLink: commonItems::parser
{
  public:
	explicit HeirLink(std::istream& theStream)
	{
		registerKeyword("ck3", [this](std::istream& stream) {
			ck3Fragment = commonItems::getString(stream);
		});
		registerKeyword("eu5", [this](std::istream& stream) {
			eu5Selection = commonItems::getString(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string ck3Fragment;
	std::string eu5Selection;
};
} // namespace

mappers::LawMapper::LawMapper(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::LawMapper::loadMappings(const std::filesystem::path& fileName)
{
	registerKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::LawMapper::registerKeys()
{
	registerKeyword("value", [this](std::istream& theStream) {
		const ValueLink link(theStream);
		if (!link.ck3Law.empty() && !link.key.empty())
			valueLinks.emplace(link.ck3Law, std::make_pair(link.key, link.offset));
	});
	registerKeyword("ethos", [this](std::istream& theStream) {
		const ShiftLink link(theStream);
		if (!link.ck3Pillar.empty() && !link.shifts.empty())
			ethosShifts.emplace(link.ck3Pillar, link.shifts);
	});
	registerKeyword("tradition", [this](std::istream& theStream) {
		const ShiftLink link(theStream);
		if (!link.ck3Pillar.empty() && !link.shifts.empty())
			traditionShifts.emplace(link.ck3Pillar, link.shifts);
	});
	registerKeyword("heir", [this](std::istream& theStream) {
		const HeirLink link(theStream);
		if (!link.ck3Fragment.empty() && !link.eu5Selection.empty())
			heirFragments.emplace_back(link.ck3Fragment, link.eu5Selection);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

std::map<std::string, int> mappers::LawMapper::getValuePositions(const std::set<std::string>& ck3Laws) const
{
	std::map<std::string, int> positions;
	for (const auto& law: ck3Laws)
		if (const auto& match = valueLinks.find(law); match != valueLinks.end())
			positions[match->second.first] = match->second.second;
	return positions;
}

std::map<std::string, int> mappers::LawMapper::getEthosShifts(const std::string& ck3Ethos) const
{
	if (const auto& match = ethosShifts.find(ck3Ethos); match != ethosShifts.end())
		return match->second;
	return {};
}

std::map<std::string, int> mappers::LawMapper::getTraditionShifts(const std::vector<std::string>& ck3Traditions) const
{
	// Two traditions pulling the same value both count; a seafaring culture of practiced pirates is
	// more naval than either alone.
	std::map<std::string, int> shifts;
	for (const auto& tradition: ck3Traditions)
		if (const auto& match = traditionShifts.find(tradition); match != traditionShifts.end())
			for (const auto& [key, shift]: match->second)
				shifts[key] += shift;
	return shifts;
}

std::optional<std::string> mappers::LawMapper::getHeirSelection(const std::set<std::string>& ck3Laws) const
{
	for (const auto& [fragment, selection]: heirFragments)
		for (const auto& law: ck3Laws)
			if (law.find(fragment) != std::string::npos)
				return selection;
	return std::nullopt;
}
