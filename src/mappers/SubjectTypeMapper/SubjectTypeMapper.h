#ifndef SUBJECT_TYPE_MAPPER_H
#define SUBJECT_TYPE_MAPPER_H
#include "Parser.h"
#include <map>
#include <string>

namespace mappers
{
// Maps CK3 tributary contract groups to EU5 subject types (configurables/subject_map.txt). A
// tributary a ruler beat into submission is not the same relationship as one that pays for
// protection, and EU5 has a subject type for each.
class SubjectTypeMapper: commonItems::parser
{
  public:
	SubjectTypeMapper() = default;
	explicit SubjectTypeMapper(std::istream& theStream);
	void loadMappings(const std::filesystem::path& fileName);

	// Falls back to a plain tributary for contract groups no rule covers.
	[[nodiscard]] std::string getSubjectType(const std::string& contractGroup) const;
	// Same-dynasty subjects are family branches rather than foreign tributaries.
	[[nodiscard]] const auto& getDynasticType() const { return dynasticType; }

  private:
	void registerKeys();

	std::map<std::string, std::string> mappings; // ck3 contract group -> eu5 subject type
	std::string defaultType = "tributary";
	std::string dynasticType = "tributary";
};
} // namespace mappers

#endif // SUBJECT_TYPE_MAPPER_H
