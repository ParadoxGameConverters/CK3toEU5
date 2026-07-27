#ifndef TITLE_TAG_MAPPER_H
#define TITLE_TAG_MAPPER_H
#include "Parser.h"
#include <map>
#include <optional>
#include <set>
#include <vector>

namespace mappers
{
// Maps CK3 titles to EU5 country tags using configurables/tag_mappings.txt.
// Titles with a mapped capital location take precedence over direct title matches.
// Unmapped titles receive generated dynamic tags (Z00, Z01, ...).
class TitleTagMapper: commonItems::parser
{
  public:
	TitleTagMapper() = default;
	explicit TitleTagMapper(std::istream& theStream);
	void loadMappings(const std::filesystem::path& fileName);

	void registerTag(const std::string& tag); // pre-register tags in use so we don't generate collisions.
	[[nodiscard]] std::optional<std::string> getTagForTitle(const std::string& ck3Title);
	[[nodiscard]] std::optional<std::string> getTagForTitle(const std::string& ck3Title, const std::string& eu5CapitalLocation);
	[[nodiscard]] auto getMappingCount() const { return static_cast<int>(titleToTag.size()); }

  private:
	void registerKeys();
	std::string generateNewTag();

	std::map<std::string, std::string> titleToTag;			 // ck3 title -> tag
	std::map<std::string, std::string> capitalToTag;		 // eu5 location -> tag
	std::map<std::string, std::string> assignedTitleTags; // cache of already-decided titles.
	std::set<std::string> usedTags;
	int generatedTagCounter = 0;
};
} // namespace mappers

#endif // TITLE_TAG_MAPPER_H
