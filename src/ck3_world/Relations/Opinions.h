#ifndef CK3_OPINIONS_H
#define CK3_OPINIONS_H
#include "Parser.h"
#include <vector>

namespace CK3
{
// Parses the savegame's opinions database for scripted relations we convert:
// rival/nemesis pairs. (Alliances live in the separate relations block.)
class Opinions: commonItems::parser
{
  public:
	Opinions() = default;
	explicit Opinions(std::istream& theStream);

	[[nodiscard]] const auto& getRivalPairs() const { return rivalPairs; }

  private:
	void registerKeys();

	std::vector<std::pair<long long, long long>> rivalPairs;
};
} // namespace CK3

#endif // CK3_OPINIONS_H
