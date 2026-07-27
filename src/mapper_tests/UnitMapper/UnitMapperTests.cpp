#include "src/mappers/UnitMapper/UnitMapper.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <sstream>

TEST(Mappers_UnitMapperTests, unknownCategoryWithoutFeudalFallbackYieldsFootmen)
{
	std::stringstream input;
	const mappers::UnitMapper mapper(input);

	const auto regiments = mapper.getRegiments("horde", 4);
	ASSERT_EQ(4u, regiments.size());
	for (const auto& regiment: regiments)
		EXPECT_EQ("a_footmen", regiment);
}

TEST(Mappers_UnitMapperTests, ratiosSplitTheArmyProportionally)
{
	std::stringstream input;
	input << "link = { category = feudal unit = a_footmen ratio = 3 unit = a_archers ratio = 1 unit = a_horsemen ratio = 1 }\n";
	const mappers::UnitMapper mapper(input);

	const auto regiments = mapper.getRegiments("feudal", 10);
	ASSERT_EQ(10u, regiments.size());
	EXPECT_EQ(6, std::ranges::count(regiments, "a_footmen")); // 3/5 of 10 = 6
	EXPECT_EQ(2, std::ranges::count(regiments, "a_archers"));
	EXPECT_EQ(2, std::ranges::count(regiments, "a_horsemen"));
}

TEST(Mappers_UnitMapperTests, roundingRemainderGoesToTheMainUnit)
{
	std::stringstream input;
	input << "link = { category = horde unit = a_horsemen ratio = 3 unit = a_footmen ratio = 1 unit = a_archers ratio = 1 }\n";
	const mappers::UnitMapper mapper(input);

	const auto regiments = mapper.getRegiments("horde", 7);
	ASSERT_EQ(7u, regiments.size());
	// 7*3/5=4, 7*1/5=1, 7*1/5=1 -> 6 dealt, remainder tops up the main unit.
	EXPECT_EQ(5, std::ranges::count(regiments, "a_horsemen"));
	EXPECT_EQ(1, std::ranges::count(regiments, "a_footmen"));
	EXPECT_EQ(1, std::ranges::count(regiments, "a_archers"));
}

TEST(Mappers_UnitMapperTests, missingCategoryFallsBackToFeudal)
{
	std::stringstream input;
	input << "link = { category = feudal unit = a_footmen ratio = 1 }\n";
	const mappers::UnitMapper mapper(input);

	const auto regiments = mapper.getRegiments("tribal", 3);
	ASSERT_EQ(3u, regiments.size());
	EXPECT_EQ(3, std::ranges::count(regiments, "a_footmen"));
}

TEST(Mappers_UnitMapperTests, nonPositiveCountsYieldNothing)
{
	std::stringstream input;
	input << "link = { category = feudal unit = a_footmen ratio = 1 }\n";
	const mappers::UnitMapper mapper(input);

	EXPECT_TRUE(mapper.getRegiments("feudal", 0).empty());
	EXPECT_TRUE(mapper.getRegiments("feudal", -5).empty());
}
