#include "src/mappers/SubjectTypeMapper/SubjectTypeMapper.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(Mappers_SubjectTypeMapperTests, contractGroupsResolveToSubjectTypes)
{
	std::stringstream input;
	input << "link = { eu5 = vassal ck3 = tributary_subjugated }\n";
	input << "link = { eu5 = dominion ck3 = tributary_mandala ck3 = tributary_wanua }\n";
	const mappers::SubjectTypeMapper mapper(input);

	EXPECT_EQ("vassal", mapper.getSubjectType("tributary_subjugated"));
	EXPECT_EQ("dominion", mapper.getSubjectType("tributary_mandala"));
	EXPECT_EQ("dominion", mapper.getSubjectType("tributary_wanua"));
}

TEST(Mappers_SubjectTypeMapperTests, unmappedGroupsFallBackToTheDefault)
{
	std::stringstream input;
	input << "default = tributary\n";
	input << "link = { eu5 = vassal ck3 = tributary_subjugated }\n";
	const mappers::SubjectTypeMapper mapper(input);

	EXPECT_EQ("tributary", mapper.getSubjectType("tributary_modded"));
	EXPECT_EQ("tributary", mapper.getSubjectType(""));
}

TEST(Mappers_SubjectTypeMapperTests, dynasticSubjectsHaveTheirOwnType)
{
	std::stringstream input;
	input << "dynastic = appanage\n";
	const mappers::SubjectTypeMapper mapper(input);

	EXPECT_EQ("appanage", mapper.getDynasticType());
}
