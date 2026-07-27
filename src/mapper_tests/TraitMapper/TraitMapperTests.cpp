#include "src/mappers/TraitMapper/TraitMapper.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(Mappers_TraitMapperTests, unmatchedTraitReturnsNothing)
{
	std::stringstream input;
	const mappers::TraitMapper mapper(input);

	EXPECT_FALSE(mapper.getEU5TraitForCK3Trait("brave"));
}

TEST(Mappers_TraitMapperTests, multipleCK3TraitsMapToOneEU5Trait)
{
	std::stringstream input;
	input << "link = { eu5 = cruel ck3 = wrathful ck3 = sadistic }\n";
	const mappers::TraitMapper mapper(input);

	EXPECT_EQ("cruel", *mapper.getEU5TraitForCK3Trait("wrathful"));
	EXPECT_EQ("cruel", *mapper.getEU5TraitForCK3Trait("sadistic"));
	EXPECT_FALSE(mapper.getEU5TraitForCK3Trait("cruel"));
}

TEST(Mappers_TraitMapperTests, linksWithoutEU5TraitAreIgnored)
{
	std::stringstream input;
	input << "link = { ck3 = brave }\n";
	const mappers::TraitMapper mapper(input);

	EXPECT_FALSE(mapper.getEU5TraitForCK3Trait("brave"));
}
