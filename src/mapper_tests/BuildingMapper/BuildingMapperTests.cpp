#include "src/mappers/BuildingMapper/BuildingMapper.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(Mappers_BuildingMapperTests, unmatchedBuildingReturnsNothing)
{
	std::stringstream input;
	const mappers::BuildingMapper mapper(input);

	EXPECT_FALSE(mapper.getEU5BuildingForCK3Building("holding_castle_01"));
}

TEST(Mappers_BuildingMapperTests, fragmentsMatchBySubstring)
{
	std::stringstream input;
	input << "link = { eu5 = temple ck3 = temple ck3 = church ck3 = mosque }\n";
	const mappers::BuildingMapper mapper(input);

	EXPECT_EQ("temple", *mapper.getEU5BuildingForCK3Building("grand_mosque_of_cordoba"));
	EXPECT_EQ("temple", *mapper.getEU5BuildingForCK3Building("church_monastery_02"));
	EXPECT_FALSE(mapper.getEU5BuildingForCK3Building("royal_mint"));
}

TEST(Mappers_BuildingMapperTests, firstMatchingLinkWins)
{
	std::stringstream input;
	input << "link = { eu5 = university ck3 = university }\n";
	input << "link = { eu5 = temple ck3 = madrasa ck3 = university }\n";
	const mappers::BuildingMapper mapper(input);

	EXPECT_EQ("university", *mapper.getEU5BuildingForCK3Building("great_university_01"));
	EXPECT_EQ("temple", *mapper.getEU5BuildingForCK3Building("madrasa_02"));
}
