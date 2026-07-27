#include "GovernmentMapper.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"

namespace
{
class GovernmentLink: commonItems::parser
{
  public:
	explicit GovernmentLink(std::istream& theStream)
	{
		registerKeyword("gov", [this](std::istream& stream) {
			category = commonItems::getString(stream);
		});
		registerKeyword("religion_group", [this](std::istream& stream) {
			religionGroup = commonItems::getString(stream);
		});
		// Repeatable: a link serving several religions lists them one `religion = x` at a time.
		registerKeyword("religion", [this](std::istream& stream) {
			religions.push_back(commonItems::getString(stream));
		});
		registerKeyword("template", [this](std::istream& stream) {
			mapping.setupTemplate = commonItems::getString(stream);
		});
		registerKeyword("type", [this](std::istream& stream) {
			mapping.governmentType = commonItems::getString(stream);
		});
		registerKeyword("parliament", [this](std::istream& stream) {
			mapping.parliament = commonItems::getString(stream);
		});
		registerKeyword("heir", [this](std::istream& stream) {
			mapping.heirSelection = commonItems::getString(stream);
		});
		registerKeyword("tech", [this](std::istream& stream) {
			mapping.techLevel = commonItems::getInt(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string category;
	std::string religionGroup;
	std::vector<std::string> religions;
	mappers::GovernmentMapping mapping;
};
} // namespace

mappers::GovernmentMapper::GovernmentMapper(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::GovernmentMapper::loadMappings(const std::filesystem::path& fileName)
{
	registerKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::GovernmentMapper::registerKeys()
{
	registerKeyword("link", [this](std::istream& theStream) {
		const GovernmentLink link(theStream);
		if (link.category.empty() || link.mapping.setupTemplate.empty())
			return;
		for (const auto& religion: link.religions)
			mappings.emplace(std::make_pair(link.category, religion), link.mapping);
		if (link.religions.empty())
			groupMappings.emplace(std::make_pair(link.category, link.religionGroup), link.mapping);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

std::optional<mappers::GovernmentMapping> mappers::GovernmentMapper::getGovernment(const std::string& ck3Category,
	 const std::string& religion,
	 const std::string& religionGroup) const
{
	if (const auto& exact = mappings.find({ck3Category, religion}); exact != mappings.end())
		return exact->second;
	if (const auto& group = groupMappings.find({ck3Category, religionGroup}); group != groupMappings.end())
		return group->second;
	if (const auto& fallback = groupMappings.find({ck3Category, std::string()}); fallback != groupMappings.end())
		return fallback->second;
	return std::nullopt;
}
