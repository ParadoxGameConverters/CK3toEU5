#ifndef EU5_LOCATION_DEFINITIONS_H
#define EU5_LOCATION_DEFINITIONS_H
#include <filesystem>
#include <map>
#include <set>
#include <string>

namespace EU5
{
// Parses EU5's map_data/definitions.txt which nests continent > subcontinent > region > area > province blocks,
// with the innermost blocks holding bare lists of location names.
class LocationDefinitions
{
  public:
	LocationDefinitions() = default;

	void loadDefinitions(const std::filesystem::path& definitionsFile);
	void loadDefinitions(std::istream& theStream);
	void loadPorts(const std::filesystem::path& portsFile);

	[[nodiscard]] bool isValidLocation(const std::string& location) const { return locations.contains(location); }
	[[nodiscard]] const auto& getLocations() const { return locations; }
	[[nodiscard]] auto getLocationCount() const { return static_cast<int>(locations.size()); }
	[[nodiscard]] std::string getContinentForLocation(const std::string& location) const;
	[[nodiscard]] const auto& getContinentRegions() const { return continentRegions; }
	[[nodiscard]] const auto& getGroupLocations() const { return groupLocations; }
	// The sea zone a coastal location's port opens onto, or empty for landlocked locations.
	[[nodiscard]] std::string getPortSeaZone(const std::string& location) const;

  private:
	void parseTokens(std::istream& theStream);

	std::set<std::string> locations;
	std::map<std::string, std::string> locationContinents;			  // location -> top-level continent block
	std::map<std::string, std::set<std::string>> continentRegions; // continent -> all regions under it
	std::map<std::string, std::set<std::string>> groupLocations;	  // any named group (province/area/region/...) -> locations under it
	std::map<std::string, std::string> portSeaZones;					  // coastal location -> sea zone, from map_data/ports.csv
};
} // namespace EU5

#endif // EU5_LOCATION_DEFINITIONS_H
