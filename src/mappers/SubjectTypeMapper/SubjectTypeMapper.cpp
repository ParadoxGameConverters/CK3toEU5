#include "SubjectTypeMapper.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"
#include <vector>

namespace
{
class SubjectLink: commonItems::parser
{
  public:
	explicit SubjectLink(std::istream& theStream)
	{
		registerKeyword("eu5", [this](std::istream& stream) {
			eu5Type = commonItems::getString(stream);
		});
		registerKeyword("ck3", [this](std::istream& stream) {
			contractGroups.push_back(commonItems::getString(stream));
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string eu5Type;
	std::vector<std::string> contractGroups;
};
} // namespace

mappers::SubjectTypeMapper::SubjectTypeMapper(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::SubjectTypeMapper::loadMappings(const std::filesystem::path& fileName)
{
	registerKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::SubjectTypeMapper::registerKeys()
{
	registerKeyword("default", [this](std::istream& theStream) {
		defaultType = commonItems::getString(theStream);
	});
	registerKeyword("dynastic", [this](std::istream& theStream) {
		dynasticType = commonItems::getString(theStream);
	});
	registerKeyword("link", [this](std::istream& theStream) {
		const SubjectLink link(theStream);
		if (link.eu5Type.empty())
			return;
		for (const auto& group: link.contractGroups)
			mappings[group] = link.eu5Type;
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

std::string mappers::SubjectTypeMapper::getSubjectType(const std::string& contractGroup) const
{
	const auto mapping = mappings.find(contractGroup);
	if (mapping == mappings.end())
		return defaultType;
	return mapping->second;
}
