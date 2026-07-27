#ifndef BUILDING_MAPPER_H
#define BUILDING_MAPPER_H
#include "Parser.h"
#include <optional>
#include <string>
#include <vector>

namespace mappers
{
// Maps CK3 holding buildings to EU5 building types by name fragment (configurables/building_map.txt).
class BuildingMapper: commonItems::parser
{
  public:
	BuildingMapper() = default;
	explicit BuildingMapper(std::istream& theStream);
	void loadMappings(const std::filesystem::path& fileName);

	[[nodiscard]] std::optional<std::string> getEU5BuildingForCK3Building(const std::string& ck3Building) const;

  private:
	void registerKeys();

	std::vector<std::pair<std::string, std::string>> fragments; // (ck3 fragment, eu5 building), in file order
};
} // namespace mappers

#endif // BUILDING_MAPPER_H
