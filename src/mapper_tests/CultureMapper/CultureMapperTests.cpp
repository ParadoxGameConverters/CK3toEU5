#include "src/mappers/CultureMapper/CultureMapper.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(Mappers_CultureMapperTests, culturesMapToEU5Cultures)
{
	std::stringstream input;
	input << "link = { eu5 = lower_egyptian_culture ck3 = egyptian }\n";
	input << "link = { eu5 = songhai ck3 = sorko ck3 = gaw }\n";
	mappers::CultureMapper mapper;
	mapper.loadCultureMappingRules(input);

	EXPECT_EQ(3, mapper.getCultureMappingCount());
	EXPECT_EQ("lower_egyptian_culture", *mapper.getEU5CultureForCK3Culture("egyptian"));
	EXPECT_EQ("songhai", *mapper.getEU5CultureForCK3Culture("sorko"));
	EXPECT_EQ("songhai", *mapper.getEU5CultureForCK3Culture("gaw"));
	EXPECT_FALSE(mapper.getEU5CultureForCK3Culture("english"));
}

TEST(Mappers_CultureMapperTests, heritagesMapToCultureGroups)
{
	std::stringstream input;
	input << "link = { eu5 = amazigh_group eu5 = maghrebi_group heritage = heritage_berber }\n";
	input << "link = { eu5 = arabic_group heritage = heritage_arabic }\n";
	mappers::CultureMapper mapper;
	mapper.loadCultureGroupsMappingRules(input);

	EXPECT_EQ(2, mapper.getHeritageMappingCount());
	const auto groups = mapper.getEU5GroupsForHeritage("heritage_berber");
	ASSERT_EQ(2, groups.size());
	EXPECT_EQ("amazigh_group", groups[0]);
	EXPECT_EQ("maghrebi_group", groups[1]);
	EXPECT_TRUE(mapper.getEU5GroupsForHeritage("heritage_unknown").empty());
}

TEST(Mappers_CultureMapperTests, languagesMapToEU5Languages)
{
	std::stringstream input;
	input << "link = { eu5 = arabic_language ck3 = language_arabic }\n";
	input << "link = { eu5 = norwegian_language ck3 = language_norse name_list = name_list_norwegian }\n";
	mappers::CultureMapper mapper;
	mapper.loadLanguageMappingRules(input);

	EXPECT_EQ("arabic_language", *mapper.getEU5LanguageForCK3Language("language_arabic"));
	EXPECT_EQ("norwegian_language", *mapper.getEU5LanguageForCK3Language("language_norse"));
	EXPECT_EQ("norwegian_language", *mapper.getEU5LanguageForNameList("name_list_norwegian"));
	EXPECT_FALSE(mapper.getEU5LanguageForCK3Language("language_unknown"));
}

TEST(Mappers_CultureMapperTests, cultureGroupLinksCanCarryLanguages)
{
	std::stringstream input;
	input << "link = { eu5 = slavic_group language = language_czech }\n";
	mappers::CultureMapper mapper;
	mapper.loadCultureGroupsMappingRules(input);

	const auto groups = mapper.getEU5GroupsForLanguage("language_czech");
	ASSERT_EQ(1, groups.size());
	EXPECT_EQ("slavic_group", groups[0]);
}
