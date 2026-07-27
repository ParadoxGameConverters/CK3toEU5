#ifndef MEN_AT_ARMS_MAPPER_H
#define MEN_AT_ARMS_MAPPER_H
#include "Parser.h"
#include <map>
#include <optional>
#include <string>

namespace mappers
{
// Maps CK3 men-at-arms types to EU5 unit types (configurables/maa_map.txt). Siege equipment maps
// to the special target "drop" - EU5's first age has no artillery units.
class MenAtArmsMapper: commonItems::parser
{
  public:
	MenAtArmsMapper() = default;
	explicit MenAtArmsMapper(std::istream& theStream);
	void loadMappings(const std::filesystem::path& fileName);

	// Returns the EU5 unit type, std::nullopt if the type converts to nothing (siege equipment).
	// Unknown types fall back to archers so no army silently loses men.
	[[nodiscard]] std::optional<std::string> getUnitForMAA(const std::string& maaType) const;

  private:
	void registerKeys();

	std::map<std::string, std::string> mappings; // ck3 maa type -> eu5 unit type (or "drop")
};
} // namespace mappers

#endif // MEN_AT_ARMS_MAPPER_H
