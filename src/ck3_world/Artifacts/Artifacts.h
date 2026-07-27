#ifndef CK3_ARTIFACTS_H
#define CK3_ARTIFACTS_H
#include "Date.h"
#include "Parser.h"
#include "src/ck3_world/SaveMarkup.h"
#include <map>
#include <optional>
#include <vector>

namespace CK3
{
struct Artifact
{
	long long ID = 0;
	std::string name;					 // display name ("The River is Red")
	std::string description;		 // the flavour text CK3 generated for it
	std::string visualType;			 // scroll/book/crown/sword/statue... - drives the EU5 work-of-art type
	std::string rarity;				 // common/masterwork/famed/illustrious
	long long owner = 0;				 // owning character
	int quality = 0;
	int wealth = 0;
	std::optional<date> creationDate; // when the artifact was made, from its history
};

// Parses the savegame's artifacts registry: artifacts = { artifacts = { id = { ... } ... } }
class Artifacts: commonItems::parser
{
  public:
	Artifacts() = default;
	explicit Artifacts(std::istream& theStream);

	[[nodiscard]] const auto& getArtifacts() const { return artifacts; }

  private:
	void registerKeys();

	std::vector<Artifact> artifacts;
};
} // namespace CK3

#endif // CK3_ARTIFACTS_H
