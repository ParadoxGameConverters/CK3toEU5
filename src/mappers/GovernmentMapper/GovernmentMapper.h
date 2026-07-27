#ifndef GOVERNMENT_MAPPER_H
#define GOVERNMENT_MAPPER_H
#include "Parser.h"
#include <map>
#include <optional>
#include <string>

namespace mappers
{
// Everything EU5 needs to know about a converted government, resolved from configurables/government_map.txt.
// The template's own government type and parliament are mirrored here so the country entry we write
// never contradicts the template it includes (mismatches spam "invalid law" errors in game).
struct GovernmentMapping
{
	std::string governmentType; // eu5 government type: monarchy/republic/theocracy/tribe/steppe_horde
	std::string setupTemplate;	 // main_menu/setup/templates file to include
	std::string parliament;		 // parliament_type matching the template
	std::string heirSelection;	 // default heir_selection, overridden by CK3 succession laws downstream
	int techLevel = 3;			 // starting_technology_level
};

// Maps a converted CK3 government category + the country's religion to an EU5 government setup.
// Templates carry laws and estate privileges gated on specific religions - patriarchate_law is
// Orthodox-only, iqta_law is Muslim-only, clergy_land_rights is barred to Christians - so a link can
// name an exact religion, its group, or neither. The most specific match wins.
class GovernmentMapper: commonItems::parser
{
  public:
	GovernmentMapper() = default;
	explicit GovernmentMapper(std::istream& theStream);
	void loadMappings(const std::filesystem::path& fileName);

	[[nodiscard]] std::optional<GovernmentMapping> getGovernment(const std::string& ck3Category,
		 const std::string& religion,
		 const std::string& religionGroup) const;

  private:
	void registerKeys();

	std::map<std::pair<std::string, std::string>, GovernmentMapping> mappings; // (category, religion) -> mapping, for exact-religion links
	std::map<std::pair<std::string, std::string>, GovernmentMapping> groupMappings; // (category, religion group) -> mapping; group "" is the fallback
};
} // namespace mappers

#endif // GOVERNMENT_MAPPER_H
