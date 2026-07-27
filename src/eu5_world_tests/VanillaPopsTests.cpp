#include "src/eu5_world/VanillaPops.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(EU5World_VanillaPopsTests, dominantPopIsLargest)
{
	std::stringstream input;
	input << "locations={\n";
	input << "stockholm = {\n";
	input << "\tdefine_pop = {	type = nobles	size = 0.031	culture = swedish	religion = catholic }\n";
	input << "\tdefine_pop = {	type = burghers	size = 0.740	culture = holsatian	religion = catholic }\n";
	input << "\tdefine_pop = {	type = peasants	size = 18.908	culture = swedish	religion = catholic }\n";
	input << "}\n";
	input << "}\n";
	EU5::VanillaPops pops;
	pops.loadPops(input);

	EXPECT_EQ(1, pops.getLocationCount());
	const auto dominant = pops.getDominantPop("stockholm");
	ASSERT_TRUE(dominant);
	EXPECT_EQ("swedish", dominant->culture);
	EXPECT_EQ("catholic", dominant->religion);
	EXPECT_FALSE(pops.getDominantPop("nowhere"));
}
