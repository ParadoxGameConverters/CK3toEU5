#ifndef RELIGION_MAPPER_H
#define RELIGION_MAPPER_H
#include "Parser.h"
#include <map>
#include <optional>
#include <string>

namespace mappers
{
struct ReligionMapping
{
	std::string eu5Religion;
	std::optional<std::string> school; // Muslim theological school, where applicable.
};

// Maps CK3 faiths to EU5 religions using configurables/religion_map.txt.
// Links keyed on religious_head take precedence over faith-name links.
class ReligionMapper: commonItems::parser
{
  public:
	ReligionMapper() = default;
	explicit ReligionMapper(std::istream& theStream);
	void loadMappings(const std::filesystem::path& fileName);

	[[nodiscard]] std::optional<ReligionMapping> getEU5ReligionForCK3Faith(const std::string& ck3Faith, const std::string& religiousHeadTitle) const;

  private:
	void registerKeys();

	std::map<std::string, ReligionMapping> faithToReligion;			// ck3 faith name -> mapping
	std::map<std::string, ReligionMapping> religiousHeadToReligion; // ck3 title -> mapping
};
} // namespace mappers

#endif // RELIGION_MAPPER_H
