#include "src/eu5_world/Advances.h"
#include "gtest/gtest.h"
#include <sstream>

namespace
{
// A slice of the age of traditions, shaped like the real thing: a declared level at the root, a
// couple of advances that inherit theirs, and one reserved for a single country.
EU5::Advances loadTree()
{
	std::stringstream input;
	input << "written_alphabet = {\n\tage = age_1_traditions\n\tstarting_technology_level = 2\n}\n";
	input << "mapmaking = {\n\trequires = written_alphabet\n\tunlock_law = immigration_law\n}\n";
	input << "agriculture_advance = {\n\tstarting_technology_level = 1\n\tunlock_law = tillage_law\n}\n";
	input << "codified_laws = {\n\trequires = written_alphabet\n\tstarting_technology_level = 2\n}\n";
	input << "state_administration = {\n\trequires = codified_laws\n\tunlock_law = administrative_system\n}\n";
	input << "gre_own_road = {\n\tpotential = { has_or_had_tag = GRE }\n\trequires = codified_laws\n";
	input << "\tunlock_law = tillage_law\n}\n";
	EU5::Advances advances;
	advances.loadAdvancesFromStream(input);
	return advances;
}
} // namespace

TEST(EU5World_AdvancesTests, anAdvanceWithoutItsOwnLevelInheritsFromWhatItNeeds)
{
	const auto advances = loadTree();

	EXPECT_EQ(6, advances.getAdvanceCount());
	// mapmaking declares nothing, but sits on written_alphabet's level 2.
	EXPECT_EQ(2, advances.getLawTechLevel("immigration_law"));
	// Two steps up from a level-2 root is still level 2.
	EXPECT_EQ(2, advances.getLawTechLevel("administrative_system"));
	// A law two advances unlock costs the cheaper one, and Greece's private road is not a route
	// anyone else can take.
	EXPECT_EQ(1, advances.getLawTechLevel("tillage_law"));
}

TEST(EU5World_AdvancesTests, unknownLawsCostNothing)
{
	const auto advances = loadTree();

	EXPECT_EQ(0, advances.getLawTechLevel("marriage_law"));
}

TEST(EU5World_AdvancesTests, privilegesFollowTheAdvanceTheyAskFor)
{
	auto advances = loadTree();
	std::stringstream privileges;
	privileges << "clergy_literacy_rights = {\n\tpotential = { has_advance = written_alphabet }\n}\n";
	privileges << "allow_hunting = {\n\tpotential = {\n\t}\n}\n";
	advances.loadPrivilegesFromStream(privileges);

	EXPECT_EQ(2, advances.getPrivilegeTechLevel("clergy_literacy_rights"));
	EXPECT_EQ(0, advances.getPrivilegeTechLevel("allow_hunting"));
	EXPECT_EQ(0, advances.getPrivilegeTechLevel("nothing_of_the_sort"));
}
