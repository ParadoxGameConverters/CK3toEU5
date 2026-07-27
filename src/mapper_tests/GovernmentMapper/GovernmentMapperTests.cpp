#include "src/mappers/GovernmentMapper/GovernmentMapper.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(Mappers_GovernmentMapperTests, unmatchedCategoryReturnsNothing)
{
	std::stringstream input;
	const mappers::GovernmentMapper mapper(input);

	EXPECT_FALSE(mapper.getGovernment("monarchy", "catholic", "christian"));
}

TEST(Mappers_GovernmentMapperTests, exactReligionBeatsItsGroup)
{
	std::stringstream input;
	input << "link = { gov = monarchy religion = orthodox template = eastern_european_monarchy type = monarchy parliament = council heir = agnatic tech = 3 }\n";
	input << "link = { gov = monarchy religion_group = christian template = catholic_monarchy type = monarchy parliament = estate_parliament heir = primo tech "
				 "= 3 }\n";
	const mappers::GovernmentMapper mapper(input);

	const auto orthodox = mapper.getGovernment("monarchy", "orthodox", "christian");
	ASSERT_TRUE(orthodox);
	EXPECT_EQ("eastern_european_monarchy", orthodox->setupTemplate);
	EXPECT_EQ("council", orthodox->parliament);

	const auto catholic = mapper.getGovernment("monarchy", "catholic", "christian");
	ASSERT_TRUE(catholic);
	EXPECT_EQ("catholic_monarchy", catholic->setupTemplate);
}

TEST(Mappers_GovernmentMapperTests, oneLinkCanServeSeveralReligions)
{
	std::stringstream input;
	input << "link = { gov = monarchy religion = orthodox religion = nestorianism template = eastern_european_monarchy type = monarchy }\n";
	const mappers::GovernmentMapper mapper(input);

	ASSERT_TRUE(mapper.getGovernment("monarchy", "orthodox", "christian"));
	ASSERT_TRUE(mapper.getGovernment("monarchy", "nestorianism", "christian"));
	EXPECT_FALSE(mapper.getGovernment("monarchy", "catholic", "christian"));
}

TEST(Mappers_GovernmentMapperTests, missingGroupFallsBackToGroupless)
{
	std::stringstream input;
	input << "link = { gov = monarchy religion_group = muslim template = muslim_monarchy type = monarchy }\n";
	input << "link = { gov = monarchy template = catholic_monarchy type = monarchy parliament = estate_parliament heir = primo tech = 2 }\n";
	const mappers::GovernmentMapper mapper(input);

	const auto fallback = mapper.getGovernment("monarchy", "mahayana", "buddhist");
	ASSERT_TRUE(fallback);
	EXPECT_EQ("catholic_monarchy", fallback->setupTemplate);
	EXPECT_EQ("monarchy", fallback->governmentType);
	EXPECT_EQ(2, fallback->techLevel);
}

TEST(Mappers_GovernmentMapperTests, linksWithoutTemplateAreIgnored)
{
	std::stringstream input;
	input << "link = { gov = republic type = republic }\n";
	const mappers::GovernmentMapper mapper(input);

	EXPECT_FALSE(mapper.getGovernment("republic", "", ""));
}
