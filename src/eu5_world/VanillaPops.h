#ifndef EU5_VANILLA_POPS_H
#define EU5_VANILLA_POPS_H
#include "Parser.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace EU5
{
struct DominantPop
{
	std::string culture;
	std::string religion;
};

struct Pop
{
	std::string type;
	double size = 0;
	std::string culture;
	std::string religion;
};

// Parses EU5's main_menu/setup/start/06_pops.txt, keeping every location's full pop list.
// Used to reculture pops for converted locations and as a fallback source for converted
// countries' culture/religion when the CK3 data cannot be mapped directly.
class VanillaPops: commonItems::parser
{
  public:
	VanillaPops() = default;

	void loadPops(const std::filesystem::path& popsFile);
	void loadPops(std::istream& theStream);

	[[nodiscard]] std::optional<DominantPop> getDominantPop(const std::string& location) const;
	[[nodiscard]] const auto& getLocationPops() const { return locationPops; }
	[[nodiscard]] auto getLocationCount() const { return static_cast<int>(dominantPops.size()); }

  private:
	void registerKeys();

	std::map<std::string, DominantPop> dominantPops;	 // location -> largest pop's culture/religion
	std::map<std::string, std::vector<Pop>> locationPops; // location -> all pops, in file order
};
} // namespace EU5

#endif // EU5_VANILLA_POPS_H
