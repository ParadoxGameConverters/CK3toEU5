#ifndef CK3_CULTURE_H
#define CK3_CULTURE_H
#include "Color.h"
#include "Parser.h"
#include <set>
extern commonItems::Color::Factory laFabricaDeColor;

namespace CK3
{
class Culture: commonItems::parser
{
  public:
	Culture() = default;
	Culture(std::istream& theStream, long long theID);

	[[nodiscard]] auto getID() const { return ID; }
	[[nodiscard]] auto isDynamic() const { return dynamic; }
	[[nodiscard]] const auto& getLocalizedName() const { return localizedName; }
	[[nodiscard]] const auto& getName() const { return name; }
	[[nodiscard]] const auto& getNameLists() const { return nameLists; }
	[[nodiscard]] const auto& getHeritage() const { return heritage; }
	[[nodiscard]] const auto& getLanguage() const { return language; }
	[[nodiscard]] const auto& getColor() const { return color; }
	[[nodiscard]] const auto& getTemplate() const { return culture_template; }
	[[nodiscard]] const auto& getEthos() const { return ethos; }
	[[nodiscard]] const auto& getTraditions() const { return traditions; }
	[[nodiscard]] const auto& getEra() const { return era; }
	[[nodiscard]] auto getInnovationCount() const { return innovationCount; }

	void setDynamic() { dynamic = true; }

  private:
	void registerKeys();

	long long ID = 0;
	bool dynamic = false; // this culture is dynamic (hybrid/divergence) and has no vanilla template

	std::optional<std::string> culture_template; // this has data only for base ck3 cultures, like czech or german
	std::optional<std::string> localizedName;		// this can be anything - user input or localized name in a particular language game is running.
	std::string heritage;								// all cultures should have this.
	std::string language;								// all cultures should have this. Used for EU5 language mapping.
	std::optional<commonItems::Color> color;		// the color the culture wore on the CK3 map
	std::set<std::string> nameLists;					// We use these to generate dynamic culture code names, in lack of a better solution.
	std::string ethos;
	std::vector<std::string> traditions;
	std::string era;			  // culture_era_tribal / _early_medieval / _high_medieval / _late_medieval
	int innovationCount = 0; // innovations the culture has unlocked, across all eras

	std::string name; // calculated value: vanilla template name if present, otherwise localized/generated name.
};
} // namespace CK3

#endif // CK3_CULTURE_H
