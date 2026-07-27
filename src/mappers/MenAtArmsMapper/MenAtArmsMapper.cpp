#include "MenAtArmsMapper.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"
#include <vector>

namespace
{
class MAALink: commonItems::parser
{
  public:
	explicit MAALink(std::istream& theStream)
	{
		registerKeyword("eu5", [this](std::istream& stream) {
			eu5Unit = commonItems::getString(stream);
		});
		registerKeyword("ck3", [this](std::istream& stream) {
			ck3Types.push_back(commonItems::getString(stream));
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string eu5Unit;
	std::vector<std::string> ck3Types;
};
} // namespace

mappers::MenAtArmsMapper::MenAtArmsMapper(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::MenAtArmsMapper::loadMappings(const std::filesystem::path& fileName)
{
	registerKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::MenAtArmsMapper::registerKeys()
{
	registerKeyword("link", [this](std::istream& theStream) {
		const MAALink link(theStream);
		if (link.eu5Unit.empty())
			return;
		for (const auto& maaType: link.ck3Types)
			mappings[maaType] = link.eu5Unit;
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

std::optional<std::string> mappers::MenAtArmsMapper::getUnitForMAA(const std::string& maaType) const
{
	const auto mapping = mappings.find(maaType);
	if (mapping == mappings.end())
		return "a_archers"; // modded or unknown types still field men rather than vanish
	if (mapping->second == "drop")
		return std::nullopt;
	return mapping->second;
}
