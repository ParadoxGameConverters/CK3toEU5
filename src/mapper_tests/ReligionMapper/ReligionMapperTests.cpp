#include "src/mappers/ReligionMapper/ReligionMapper.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(Mappers_ReligionMapperTests, unmatchedFaithReturnsNothing)
{
	std::stringstream input;
	const mappers::ReligionMapper mapper(input);

	EXPECT_FALSE(mapper.getEU5ReligionForCK3Faith("catholic", ""));
}

TEST(Mappers_ReligionMapperTests, faithsCanBeMatchedByName)
{
	std::stringstream input;
	input << "link = { eu5 = mahayana ck3 = mahayana ck3 = ari ck3 = avatamsaka }\n";
	const mappers::ReligionMapper mapper(input);

	EXPECT_EQ("mahayana", mapper.getEU5ReligionForCK3Faith("ari", "")->eu5Religion);
	EXPECT_FALSE(mapper.getEU5ReligionForCK3Faith("ari", "")->school);
}

TEST(Mappers_ReligionMapperTests, religiousHeadTakesPrecedence)
{
	std::stringstream input;
	input << "link = { eu5 = catholic religious_head = k_papal_state }\n";
	input << "link = { eu5 = bogomilism ck3 = my_custom_faith }\n";
	const mappers::ReligionMapper mapper(input);

	// A custom faith whose head is the pope maps to catholic despite its name mapping elsewhere.
	EXPECT_EQ("catholic", mapper.getEU5ReligionForCK3Faith("my_custom_faith", "k_papal_state")->eu5Religion);
	// Without the head it falls back to the name link.
	EXPECT_EQ("bogomilism", mapper.getEU5ReligionForCK3Faith("my_custom_faith", "")->eu5Religion);
}

TEST(Mappers_ReligionMapperTests, muslimSchoolsAreCarried)
{
	std::stringstream input;
	input << "link = { eu5 = sunni ck3 = ashari school = ashari_school }\n";
	const mappers::ReligionMapper mapper(input);

	const auto& mapping = mapper.getEU5ReligionForCK3Faith("ashari", "");
	ASSERT_TRUE(mapping);
	EXPECT_EQ("sunni", mapping->eu5Religion);
	EXPECT_EQ("ashari_school", *mapping->school);
}
