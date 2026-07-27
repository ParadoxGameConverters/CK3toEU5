#ifndef TRAIT_MAPPER_H
#define TRAIT_MAPPER_H
#include "Parser.h"
#include <map>
#include <optional>
#include <string>

namespace mappers
{
// Maps CK3 personality/education traits to EU5 ruler traits, and CK3 martial traits to the separate
// EU5 general traits army commanders carry (configurables/trait_map.txt).
class TraitMapper: commonItems::parser
{
  public:
	TraitMapper() = default;
	explicit TraitMapper(std::istream& theStream);
	void loadMappings(const std::filesystem::path& fileName);

	[[nodiscard]] std::optional<std::string> getEU5TraitForCK3Trait(const std::string& ck3Trait) const;
	[[nodiscard]] std::optional<std::string> getGeneralTraitForCK3Trait(const std::string& ck3Trait) const;

  private:
	void registerKeys();

	std::map<std::string, std::string> traitMap;			// ck3 trait -> eu5 ruler trait
	std::map<std::string, std::string> generalTraitMap; // ck3 trait -> eu5 general trait
};
} // namespace mappers

#endif // TRAIT_MAPPER_H
