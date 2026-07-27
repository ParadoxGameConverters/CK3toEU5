#include "Armies.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"
#include <sstream>

namespace
{
// One regiment entry. Men-at-arms have a type, an owner and a size; levies have neither type nor
// owner; mercenary stock is marked source=hired.
class RegimentEntry: commonItems::parser
{
  public:
	explicit RegimentEntry(std::istream& theStream)
	{
		registerKeyword("type", [this](std::istream& stream) {
			type = commonItems::getString(stream);
		});
		registerKeyword("owner", [this](std::istream& stream) {
			owner = commonItems::getLlong(stream);
		});
		registerKeyword("size", [this](std::istream& stream) {
			size = static_cast<int>(commonItems::getLlong(stream));
		});
		registerKeyword("max", [this](std::istream& stream) {
			max = static_cast<int>(commonItems::getLlong(stream));
		});
		registerKeyword("source", [this](std::istream& stream) {
			source = commonItems::getString(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string type;
	std::string source;
	long long owner = 0;
	int size = 0;
	int max = 0;
};
} // namespace

CK3::Armies::Armies(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void CK3::Armies::registerKeys()
{
	registerKeyword("regiments", [this](std::istream& theStream) {
		commonItems::parser regimentsParser;
		regimentsParser.registerRegex(commonItems::catchallRegex, [this]([[maybe_unused]] const std::string& regimentID, std::istream& stream) {
			// Entries are either <id> = { ... } or <id> = none for cleared slots.
			const auto token = commonItems::stringOfItem(stream).getString();
			if (token.find('{') == std::string::npos)
				return;
			std::stringstream tokenStream(token);
			const RegimentEntry entry(tokenStream);
			if (entry.type.empty() || entry.owner <= 0 || entry.source == "hired")
				return;
			const auto men = entry.size > 0 ? entry.size : entry.max;
			if (men <= 0)
				return;
			menAtArms[entry.owner][entry.type] += men;
			++regimentCount;
		});
		regimentsParser.parseStream(theStream);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}
