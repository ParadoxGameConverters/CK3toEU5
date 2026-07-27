#include "src/mappers/TitleTagMapper/TitleTagMapper.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(Mappers_TitleTagMapperTests, emptyTitleReturnsNothing)
{
	std::stringstream input;
	mappers::TitleTagMapper mapper(input);

	EXPECT_FALSE(mapper.getTagForTitle(""));
}

TEST(Mappers_TitleTagMapperTests, titlesCanBeMatched)
{
	std::stringstream input;
	input << "link = { ck3 = k_denmark eu5 = DAN }\n";
	input << "link = { ck3 = d_danes eu5 = DAN }\n";
	mappers::TitleTagMapper mapper(input);

	EXPECT_EQ("DAN", *mapper.getTagForTitle("k_denmark"));
	EXPECT_EQ("DAN", *mapper.getTagForTitle("d_danes"));
}

TEST(Mappers_TitleTagMapperTests, capitalsTakePrecedenceOverTitles)
{
	std::stringstream input;
	input << "link = { ck3 = c_vestisland eu5 = VST capitals = { keflavik reikjavik } }\n";
	input << "link = { ck3 = e_mongols eu5 = MON }\n";
	mappers::TitleTagMapper mapper(input);

	EXPECT_EQ("VST", *mapper.getTagForTitle("e_mongols", "keflavik"));
}

TEST(Mappers_TitleTagMapperTests, unmappedTitlesReceiveGeneratedTags)
{
	std::stringstream input;
	input << "link = { ck3 = k_denmark eu5 = DAN }\n";
	mappers::TitleTagMapper mapper(input);

	EXPECT_EQ("Z00", *mapper.getTagForTitle("x_dynamic_title_1"));
	EXPECT_EQ("Z01", *mapper.getTagForTitle("x_dynamic_title_2"));
	// Same title asks again, gets the same tag.
	EXPECT_EQ("Z00", *mapper.getTagForTitle("x_dynamic_title_1"));
}

TEST(Mappers_TitleTagMapperTests, generatedTagsAvoidRegisteredCollisions)
{
	std::stringstream input;
	mappers::TitleTagMapper mapper(input);
	mapper.registerTag("Z00");

	EXPECT_EQ("Z01", *mapper.getTagForTitle("x_dynamic_title_1"));
}
