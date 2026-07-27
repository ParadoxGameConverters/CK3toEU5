#include "VassalContracts.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"
#include <sstream>

namespace
{
class ContractEntry: commonItems::parser
{
  public:
	explicit ContractEntry(std::istream& theStream)
	{
		registerKeyword("vassal", [this](std::istream& stream) {
			vassal = commonItems::getLlong(stream);
		});
		registerKeyword("contract_group", [this](std::istream& stream) {
			group = commonItems::getString(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	long long vassal = 0;
	std::string group;
};
} // namespace

CK3::VassalContracts::VassalContracts(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void CK3::VassalContracts::registerKeys()
{
	databaseParser.registerRegex(R"(\d+)", [this](const std::string&, std::istream& theStream) {
		const auto blob = commonItems::stringOfItem(theStream).getString();
		if (blob.find('{') == std::string::npos)
			return; // a cancelled contract, written as "id=none"
		auto blobStream = std::stringstream(blob);
		const ContractEntry entry(blobStream);
		if (entry.vassal == 0 || entry.group.empty())
			return;
		contractGroups[entry.vassal] = entry.group;
	});
	databaseParser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

	registerKeyword("database", [this](std::istream& theStream) {
		databaseParser.parseStream(theStream);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

std::string CK3::VassalContracts::getContractGroup(long long vassalID) const
{
	const auto& group = contractGroups.find(vassalID);
	if (group == contractGroups.end())
		return {};
	return group->second;
}
