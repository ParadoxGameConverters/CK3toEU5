#ifndef CK3_RELATIONS_H
#define CK3_RELATIONS_H
#include "Parser.h"
#include <vector>

namespace CK3
{
// Parses the savegame's relations database, keeping only what we can use downstream:
// pairs of characters bound by an active alliance.
class Relations: commonItems::parser
{
  public:
	Relations() = default;
	explicit Relations(std::istream& theStream);

	[[nodiscard]] const auto& getAlliancePairs() const { return alliancePairs; }

  private:
	void registerKeys();

	std::vector<std::pair<long long, long long>> alliancePairs;
};
} // namespace CK3

#endif // CK3_RELATIONS_H
