#ifndef EU5_VANILLA_TOWNS_H
#define EU5_VANILLA_TOWNS_H
#include "Parser.h"
#include <map>
#include <string>

namespace EU5
{
struct VanillaTown
{
	std::string rank;	 // town, city, megalopolis, rural_settlement
	std::string setup; // town_setup granting the starting buildings
};

// Parses the locations block of EU5's main_menu/setup/start/07_cities_and_buildings.txt. Vanilla's
// urban footprint is the yardstick the converted world is measured against: EU5's economy is
// balanced around roughly one urban location in fourteen, and its town setups are the source of a
// converted town's starting buildings.
class VanillaTowns: commonItems::parser
{
  public:
	VanillaTowns() = default;

	void loadTowns(const std::filesystem::path& citiesFile);
	void loadTowns(std::istream& theStream);

	[[nodiscard]] const auto& getTowns() const { return towns; }
	[[nodiscard]] bool isUrban(const std::string& location) const { return towns.contains(location); }
	[[nodiscard]] bool isCity(const std::string& location) const;
	[[nodiscard]] auto getTownCount() const { return static_cast<int>(towns.size()); }

  private:
	void registerKeys();

	std::map<std::string, VanillaTown> towns; // location -> rank and setup
};
} // namespace EU5

#endif // EU5_VANILLA_TOWNS_H
