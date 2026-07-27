#ifndef EU5_ADVANCES_H
#define EU5_ADVANCES_H
#include <filesystem>
#include <map>
#include <set>
#include <string>

namespace EU5
{
// EU5's age of traditions is a research tree a country starts partway up: every advance carries a
// starting_technology_level, and a country begins with each advance at or below its own level - plus,
// implicitly, everything that advance was built on. Laws and estate privileges hang off that tree,
// which is why a country handed a law its technology never unlocked is an error at load.
//
// This reads in_game/common/advances and in_game/common/estate_privileges to answer the one question
// the converter needs: how advanced does a country have to be before it is allowed this law?
class Advances
{
  public:
	Advances() = default;

	void loadAdvances(const std::filesystem::path& advancesFolder);
	void loadAdvancesFromStream(std::istream& theStream);
	void loadPrivileges(const std::filesystem::path& privilegesFolder);
	void loadPrivilegesFromStream(std::istream& theStream);

	// The lowest starting_technology_level at which a country has this law or privilege available.
	// Zero for anything available from the start, and for anything unrecognized.
	[[nodiscard]] int getLawTechLevel(const std::string& law) const;
	[[nodiscard]] int getPrivilegeTechLevel(const std::string& privilege) const;

	[[nodiscard]] auto getAdvanceCount() const { return static_cast<int>(advanceLevels.size()); }

  private:
	void readAdvanceText(const std::string& text);
	void readPrivilegeText(const std::string& text);
	// Resolves an advance's real starting level: its own, or its prerequisites' when it declares none.
	[[nodiscard]] int resolveLevel(const std::string& advance, std::set<std::string>& visiting) const;
	void resolveAll();

	struct Advance
	{
		int declaredLevel = -1; // -1 when the advance leaves it to its prerequisites
		std::set<std::string> prerequisites;
		std::set<std::string> unlockedLaws;
		bool countrySpecific = false; // gated on a tag, culture or religion, so no general route to a law
	};

	std::map<std::string, Advance> advances;
	std::map<std::string, int> advanceLevels;	 // advance -> resolved starting technology level
	std::map<std::string, int> lawLevels;		 // law -> lowest level unlocking it
	std::map<std::string, int> privilegeLevels; // estate privilege -> level of the advance it asks for
};
} // namespace EU5

#endif // EU5_ADVANCES_H
