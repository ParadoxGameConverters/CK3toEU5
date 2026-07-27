#ifndef CK3_CONFEDERATION_H
#define CK3_CONFEDERATION_H
#include "Color.h"
#include "Parser.h"
extern commonItems::Color::Factory laFabricaDeColor;

namespace CK3
{
class CoatOfArms;

// A standing bloc of houses - the steppe and Japanese confederations CK3 lets dynasties form.
// Membership is by house rather than by character, so the realms behind a bloc are found by
// looking up who each house's head rules.
class Confederation: commonItems::parser
{
  public:
	Confederation() = default;
	Confederation(std::istream& theStream, long long theID);

	[[nodiscard]] auto getID() const { return ID; }
	[[nodiscard]] auto getLeaderHouse() const { return leaderHouse; }
	[[nodiscard]] const auto& getName() const { return name; }
	[[nodiscard]] const auto& getColor() const { return color; }
	[[nodiscard]] const auto& getCoat() const { return coat; }
	[[nodiscard]] const auto& getHouses() const { return houses; }

	void loadCoat(const std::pair<long long, std::shared_ptr<CoatOfArms>>& theCoat) { coat = theCoat; }

  private:
	void registerKeys();

	long long ID = 0;
	long long leaderHouse = 0;
	std::string name;
	std::optional<commonItems::Color> color;
	std::optional<std::pair<long long, std::shared_ptr<CoatOfArms>>> coat;
	std::vector<long long> houses;
};
} // namespace CK3

#endif // CK3_CONFEDERATION_H
