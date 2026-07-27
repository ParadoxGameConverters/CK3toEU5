#include "src/ck3_world/Armies/Armies.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(CK3World_ArmiesTests, emptyInputYieldsNoMenAtArms)
{
	std::stringstream input;
	const CK3::Armies armies(input);

	EXPECT_TRUE(armies.getMenAtArms().empty());
	EXPECT_EQ(0, armies.getRegimentCount());
}

TEST(CK3World_ArmiesTests, typedOwnedRegimentsAggregatePerOwnerAndType)
{
	std::stringstream input;
	input << "regiments = {\n";
	input << "\t100 = {\n";
	input << "\t\ttype = armored_footmen\n";
	input << "\t\torigin = 4774\n";
	input << "\t\tsize = 300\n";
	input << "\t\towner = 42\n";
	input << "\t}\n";
	input << "\t101 = {\n";
	input << "\t\ttype = armored_footmen\n";
	input << "\t\tsize = 200\n";
	input << "\t\towner = 42\n";
	input << "\t}\n";
	input << "\t102 = {\n";
	input << "\t\ttype = bowmen\n";
	input << "\t\tmax = 150\n";
	input << "\t\towner = 43\n";
	input << "\t}\n";
	input << "}\n";
	const CK3::Armies armies(input);

	ASSERT_EQ(2u, armies.getMenAtArms().size());
	EXPECT_EQ(500, armies.getMenAtArms().at(42).at("armored_footmen"));
	EXPECT_EQ(150, armies.getMenAtArms().at(43).at("bowmen"));
	EXPECT_EQ(3, armies.getRegimentCount());
}

TEST(CK3World_ArmiesTests, leviesMercenariesAndClearedSlotsAreSkipped)
{
	std::stringstream input;
	input << "regiments = {\n";
	input << "\t0 = none\n";
	input << "\t1 = {\n"; // levy: no type
	input << "\t\torigin = 3003\n";
	input << "\t\tmax = 248\n";
	input << "\t}\n";
	input << "\t2 = {\n"; // mercenary stock: hired
	input << "\t\ttype = mubarizun\n";
	input << "\t\tsize = 433\n";
	input << "\t\towner = 57799\n";
	input << "\t\tsource = hired\n";
	input << "\t}\n";
	input << "\t3 = {\n"; // garrison
	input << "\t\tsize = 1312\n";
	input << "\t\torigin = 3003\n";
	input << "\t\tsource = garrison\n";
	input << "\t}\n";
	input << "}\n";
	const CK3::Armies armies(input);

	EXPECT_TRUE(armies.getMenAtArms().empty());
}
