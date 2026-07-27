#include "Confederation.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"

CK3::Confederation::Confederation(std::istream& theStream, long long theID): ID(theID)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void CK3::Confederation::registerKeys()
{
	registerKeyword("houses", [this](std::istream& theStream) {
		houses = commonItems::getLlongs(theStream);
	});
	registerKeyword("leader", [this](std::istream& theStream) {
		// Despite the bare key name, the leader is a house, matching the houses list.
		leaderHouse = commonItems::getLlong(theStream);
	});
	registerKeyword("name", [this](std::istream& theStream) {
		name = commonItems::getString(theStream);
	});
	registerKeyword("color", [this](std::istream& theStream) {
		color = laFabricaDeColor.getColor(theStream);
	});
	registerKeyword("coat_of_arms", [this](std::istream& theStream) {
		coat = std::pair(commonItems::singleLlong(theStream).getLlong(), nullptr);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}
