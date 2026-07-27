#include "src/mappers/MenAtArmsMapper/MenAtArmsMapper.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(Mappers_MenAtArmsMapperTests, mappedTypesResolveToTheirUnit)
{
	std::stringstream input;
	input << "link = { eu5 = a_footmen ck3 = armored_footmen ck3 = huscarl }\n";
	input << "link = { eu5 = a_horsemen ck3 = horse_archers }\n";
	const mappers::MenAtArmsMapper mapper(input);

	EXPECT_EQ("a_footmen", *mapper.getUnitForMAA("armored_footmen"));
	EXPECT_EQ("a_footmen", *mapper.getUnitForMAA("huscarl"));
	EXPECT_EQ("a_horsemen", *mapper.getUnitForMAA("horse_archers"));
}

TEST(Mappers_MenAtArmsMapperTests, dropTargetConvertsToNothing)
{
	std::stringstream input;
	input << "link = { eu5 = drop ck3 = trebuchet ck3 = onager }\n";
	const mappers::MenAtArmsMapper mapper(input);

	EXPECT_FALSE(mapper.getUnitForMAA("trebuchet"));
	EXPECT_FALSE(mapper.getUnitForMAA("onager"));
}

TEST(Mappers_MenAtArmsMapperTests, unknownTypesFallBackToArchers)
{
	std::stringstream input;
	input << "link = { eu5 = a_footmen ck3 = armored_footmen }\n";
	const mappers::MenAtArmsMapper mapper(input);

	EXPECT_EQ("a_archers", *mapper.getUnitForMAA("modded_special_guard"));
}
