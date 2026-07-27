#ifndef UNIT_MAPPER_H
#define UNIT_MAPPER_H
#include "Parser.h"
#include <map>
#include <string>
#include <vector>

namespace mappers
{
// Maps a converted army category (feudal/tribal/horde...) to an EU5 unit composition,
// using configurables/unit_map.txt. Ratios are relative weights within the army.
class UnitMapper: commonItems::parser
{
  public:
	UnitMapper() = default;
	explicit UnitMapper(std::istream& theStream);
	void loadMappings(const std::filesystem::path& fileName);

	// Builds a regiment list of the requested size following the category's composition ratios.
	[[nodiscard]] std::vector<std::string> getRegiments(const std::string& category, int count) const;

  private:
	void registerKeys();

	std::map<std::string, std::vector<std::pair<std::string, int>>> compositions; // category -> (unit, ratio)
};
} // namespace mappers

#endif // UNIT_MAPPER_H
