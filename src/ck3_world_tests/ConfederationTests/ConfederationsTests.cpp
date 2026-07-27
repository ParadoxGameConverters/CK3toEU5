#include "src/ck3_world/Confederations/Confederation.h"
#include "src/ck3_world/Confederations/Confederations.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(CK3World_ConfederationsTests, emptyInputYieldsNoConfederations)
{
	std::stringstream input;
	const CK3::Confederations confederations(input);

	EXPECT_TRUE(confederations.getConfederations().empty());
}

TEST(CK3World_ConfederationsTests, confederationsAreKeyedByID)
{
	std::stringstream input;
	input << "database = {\n";
	input << "\t83886080 = {\n";
	input << "\t\ttype = house_bloc_ceremony\n";
	input << "\t\tname = \"Onodera Bloc\"\n";
	input << "\t\tleader = 16790720\n";
	input << "\t\thouses = { 16790720 13172 10853 }\n";
	input << "\t}\n";
	input << "}\n";
	const CK3::Confederations confederations(input);

	ASSERT_EQ(1u, confederations.getConfederations().size());
	const auto& bloc = confederations.getConfederations().at(83886080);
	EXPECT_EQ("Onodera Bloc", bloc->getName());
	EXPECT_EQ(16790720, bloc->getLeaderHouse());
	EXPECT_EQ(3u, bloc->getHouses().size());
}

TEST(CK3World_ConfederationsTests, disbandedSlotsAreSkipped)
{
	std::stringstream input;
	input << "database = {\n";
	input << "\t50331649 = none\n";
	input << "\t7 = none\n";
	input << "}\n";
	const CK3::Confederations confederations(input);

	EXPECT_TRUE(confederations.getConfederations().empty());
}
