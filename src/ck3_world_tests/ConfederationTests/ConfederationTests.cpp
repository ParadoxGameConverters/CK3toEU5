#include "src/ck3_world/Confederations/Confederation.h"
#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"

using testing::ElementsAre;
TEST(CK3World_ConfederationTests, confederationIDLoads)
{
	std::stringstream input;
	const CK3::Confederation confederation(input, 42);

	EXPECT_EQ(42, confederation.getID());
}

TEST(CK3World_ConfederationTests, loadValuesDefaultToDefaults)
{
	std::stringstream input;
	const CK3::Confederation confederation(input, 1);

	EXPECT_EQ(1, confederation.getID());
	EXPECT_TRUE(confederation.getName().empty());
	EXPECT_FALSE(confederation.getColor());
	EXPECT_FALSE(confederation.getCoat());
	EXPECT_TRUE(confederation.getHouses().empty());
	EXPECT_EQ(0, confederation.getLeaderHouse());
}

TEST(CK3World_ConfederationTests, confederationPrimitivesCanBeLoaded)
{
	std::stringstream input;
	input << "type=house_bloc_ceremony\n";
	input << "name=\"Polabian Confederation\"\n";
	input << "cohesion=77.16667\n";
	input << "leader=16795222\n";
	input << "houses={ 16793418 16795222 16810363 26726 16799741 }\n";
	input << "color=rgb { 80 110 80 }\n";
	input << "coat_of_arms=20636\n";

	const CK3::Confederation confederation(input, 42);

	EXPECT_THAT(confederation.getHouses(), ElementsAre(16793418, 16795222, 16810363, 26726, 16799741));
	EXPECT_EQ(16795222, confederation.getLeaderHouse());
	EXPECT_EQ("Polabian Confederation", confederation.getName());
	EXPECT_EQ("= rgb { 80 110 80 }", confederation.getColor()->outputRgb());
	EXPECT_EQ(20636, confederation.getCoat()->first);
	EXPECT_FALSE(confederation.getCoat()->second);
}
