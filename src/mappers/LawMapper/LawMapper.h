#ifndef LAW_MAPPER_H
#define LAW_MAPPER_H
#include "Parser.h"
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace mappers
{
// Maps CK3 realm laws and cultural pillars to EU5 societal values and heir selection
// (configurables/law_map.txt). Laws set values outright; ethos and traditions shift them.
class LawMapper: commonItems::parser
{
  public:
	LawMapper() = default;
	explicit LawMapper(std::istream& theStream);
	void loadMappings(const std::filesystem::path& fileName);

	// Societal value positions triggered by the realm's laws, e.g. centralization_vs_decentralization -> 60.
	[[nodiscard]] std::map<std::string, int> getValuePositions(const std::set<std::string>& ck3Laws) const;
	// Relative shifts the culture's ethos and traditions apply on top of whatever the template set.
	[[nodiscard]] std::map<std::string, int> getEthosShifts(const std::string& ck3Ethos) const;
	[[nodiscard]] std::map<std::string, int> getTraditionShifts(const std::vector<std::string>& ck3Traditions) const;
	// Heir selection matching the realm's succession law, if any law matches a configured fragment.
	[[nodiscard]] std::optional<std::string> getHeirSelection(const std::set<std::string>& ck3Laws) const;

	[[nodiscard]] auto getEthosCount() const { return static_cast<int>(ethosShifts.size()); }
	[[nodiscard]] auto getTraditionCount() const { return static_cast<int>(traditionShifts.size()); }

  private:
	void registerKeys();

	std::map<std::string, std::pair<std::string, int>> valueLinks;	  // ck3 law -> (societal value, position)
	std::map<std::string, std::map<std::string, int>> ethosShifts;	  // ck3 ethos -> (societal value -> shift)
	std::map<std::string, std::map<std::string, int>> traditionShifts; // ck3 tradition -> (societal value -> shift)
	std::vector<std::pair<std::string, std::string>> heirFragments; // (ck3 fragment, eu5 heir_selection), in file order
};
} // namespace mappers

#endif // LAW_MAPPER_H
