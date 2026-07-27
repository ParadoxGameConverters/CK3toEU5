#include "Wars.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"
#include "src/ck3_world/SaveMarkup.h"
#include <sstream>

namespace
{
// A war side: attacker = { participants = { { character = X ... } ... } casualties = { ... } }
class WarSide: commonItems::parser
{
  public:
	explicit WarSide(std::istream& theStream)
	{
		registerKeyword("participants", [this](std::istream& stream) {
			for (const auto& blob: commonItems::blobList(stream).getBlobs())
			{
				std::stringstream blobStream(blob);
				commonItems::parser participantParser;
				participantParser.registerKeyword("character", [this](std::istream& charStream) {
					participants.push_back(commonItems::getLlong(charStream));
				});
				participantParser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
				participantParser.parseStream(blobStream);
			}
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::vector<long long> participants;
};

class CasusBelli: commonItems::parser
{
  public:
	explicit CasusBelli(std::istream& theStream)
	{
		registerKeyword("type", [this](std::istream& stream) {
			type = commonItems::getString(stream);
		});
		registerKeyword("attacker", [this](std::istream& stream) {
			attacker = commonItems::getLlong(stream);
		});
		registerKeyword("defender", [this](std::istream& stream) {
			defender = commonItems::getLlong(stream);
		});
		registerKeyword("targeted_titles", [this](std::istream& stream) {
			targetedTitles = commonItems::getLlongs(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string type;
	long long attacker = 0;
	long long defender = 0;
	std::vector<long long> targetedTitles;
};

class WarEntry: commonItems::parser
{
  public:
	explicit WarEntry(std::istream& theStream)
	{
		registerKeyword("name", [this](std::istream& stream) {
			// Saved war names carry CK3's link markup ("\x15ONCLICK:TITLE,123 ... \x15!") around the
			// words the player actually saw; only those words survive the trip to EU5.
			war.name = CK3::stripSaveMarkup(commonItems::getString(stream));
		});
		registerKeyword("start_date", [this](std::istream& stream) {
			war.startDate = date(commonItems::getString(stream));
		});
		registerKeyword("attacker", [this](std::istream& stream) {
			const WarSide side(stream);
			war.attackerParticipants = side.participants;
		});
		registerKeyword("defender", [this](std::istream& stream) {
			const WarSide side(stream);
			war.defenderParticipants = side.participants;
		});
		registerKeyword("casus_belli", [this](std::istream& stream) {
			const CasusBelli cb(stream);
			war.cbType = cb.type;
			war.attacker = cb.attacker;
			war.defender = cb.defender;
			war.targetedTitles = cb.targetedTitles;
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	CK3::War war;
};
} // namespace

CK3::Wars::Wars(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void CK3::Wars::registerKeys()
{
	registerKeyword("active_wars", [this](std::istream& theStream) {
		commonItems::parser warsParser;
		warsParser.registerRegex(commonItems::catchallRegex, [this]([[maybe_unused]] const std::string& warID, std::istream& stream) {
			// Entries are either <id> = { ... } or <id> = none for concluded slots.
			const auto token = commonItems::stringOfItem(stream).getString();
			if (token.find('{') == std::string::npos)
				return;
			std::stringstream tokenStream(token);
			const WarEntry entry(tokenStream);
			// Wars without an identifiable primary pair can't be converted to countries downstream.
			if (entry.war.attacker <= 0 || entry.war.defender <= 0)
				return;
			wars.push_back(entry.war);
		});
		warsParser.parseStream(theStream);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}
