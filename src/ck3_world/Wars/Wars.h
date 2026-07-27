#ifndef CK3_WARS_H
#define CK3_WARS_H
#include "Date.h"
#include "Parser.h"
#include <vector>

namespace CK3
{
struct War
{
	std::string name;								  // display name from the save, markup stripped ("Peasant Uprising")
	date startDate = date("1.1.1");
	std::string cbType;							  // ck3 casus belli type
	long long attacker = 0;						  // primary attacker character
	long long defender = 0;						  // primary defender character
	std::vector<long long> attackerParticipants; // every attacking character, primary included
	std::vector<long long> defenderParticipants;
	std::vector<long long> targetedTitles; // the titles the casus belli is fought over - the wargoal
};

// Parses the savegame's wars block: wars = { active_wars = { id = { ... } id = none ... } }
class Wars: commonItems::parser
{
  public:
	Wars() = default;
	explicit Wars(std::istream& theStream);

	[[nodiscard]] const auto& getWars() const { return wars; }

  private:
	void registerKeys();

	std::vector<War> wars;
};
} // namespace CK3

#endif // CK3_WARS_H
