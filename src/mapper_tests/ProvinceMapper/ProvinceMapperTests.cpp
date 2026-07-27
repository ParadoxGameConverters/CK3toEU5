#include "src/mappers/ProvinceMapper/ProvinceMapper.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(Mappers_ProvinceMapperTests, mappingsDefaultToEmpty)
{
	std::stringstream input;
	const mappers::ProvinceMapper mapper(input);

	EXPECT_EQ(0, mapper.getMappingCount());
	EXPECT_TRUE(mapper.getEU5Locations(1).empty());
	EXPECT_TRUE(mapper.getCK3Provinces("somewhere").empty());
}

TEST(Mappers_ProvinceMapperTests, linksCanBeLoadedAndQueriedBothWays)
{
	std::stringstream input;
	input << "0.0.0 = {\n";
	input << "\ttriangulation_pair = { srcX = 1 srcY = 2 dstX = 3 dstY = 4 }\n";
	input << "\tlink = { comment = \"Manual\" }\n";
	input << "\tlink = { ck3 = 9848 eu5 = siraya eu5 = tsou }\n";
	input << "\tlink = { ck3 = 11398 ck3 = 11399 eu5 = johor_lama }\n";
	input << "}\n";
	const mappers::ProvinceMapper mapper(input);

	EXPECT_EQ(2, mapper.getMappingCount());

	const auto& locations = mapper.getEU5Locations(9848);
	ASSERT_EQ(2, locations.size());
	EXPECT_EQ("siraya", locations[0]);
	EXPECT_EQ("tsou", locations[1]);

	EXPECT_EQ(1, mapper.getEU5Locations(11398).size());
	EXPECT_EQ("johor_lama", mapper.getEU5Locations(11399)[0]);

	const auto& provinces = mapper.getCK3Provinces("johor_lama");
	ASSERT_EQ(2, provinces.size());
	EXPECT_EQ(11398, provinces[0]);
	EXPECT_EQ(11399, provinces[1]);

	EXPECT_TRUE(mapper.getEU5Locations(12345).empty());
}
