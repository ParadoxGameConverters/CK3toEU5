#ifndef CK3_VASSAL_CONTRACTS_H
#define CK3_VASSAL_CONTRACTS_H
#include "Parser.h"
#include <map>
#include <string>

namespace CK3
{
// The savegame's vassal_contracts database. Every subject relationship in CK3 - feudal vassalage,
// celestial vassalage, tribute - is one of these, and its contract_group says which kind it is.
// Internal vassals merge into their liege's realm on conversion, so only the tributary contracts
// matter here: they are the ones binding two realms that both survive as EU5 countries.
class VassalContracts: commonItems::parser
{
  public:
	VassalContracts() = default;
	explicit VassalContracts(std::istream& theStream);

	// vassal character ID -> contract group, e.g. tributary_nomadic
	[[nodiscard]] const auto& getContractGroups() const { return contractGroups; }
	[[nodiscard]] std::string getContractGroup(long long vassalID) const;

  private:
	void registerKeys();
	parser databaseParser;

	std::map<long long, std::string> contractGroups;
};
} // namespace CK3

#endif // CK3_VASSAL_CONTRACTS_H
