#include "src/eu5_world/GameDatabase.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(EU5World_GameDatabaseTests, culturesCanBeLoaded)
{
	std::stringstream input;
	input << "swedish = { language = swedish_language culture_groups = { scandinavian_group } }\n";
	input << "danish = { language = danish_language }\n";
	EU5::GameDatabase database;
	database.loadCulturesFromStream(input);

	EXPECT_EQ(2, database.getCultureCount());
	EXPECT_TRUE(database.isValidCulture("swedish"));
	EXPECT_FALSE(database.isValidCulture("bohemian"));
}

TEST(EU5World_GameDatabaseTests, religionsCanBeLoadedWithGroups)
{
	std::stringstream input;
	input << "catholic = { group = christian color = color_catholic }\n";
	input << "sunni = { group = muslim }\n";
	EU5::GameDatabase database;
	database.loadReligionsFromStream(input);

	EXPECT_EQ(2, database.getReligionCount());
	EXPECT_TRUE(database.isValidReligion("catholic"));
	EXPECT_EQ("christian", database.getReligionGroup("catholic"));
	EXPECT_EQ("muslim", database.getReligionGroup("sunni"));
	EXPECT_TRUE(database.getReligionGroup("bogus").empty());
}

TEST(EU5World_GameDatabaseTests, languageReferencesResolveToLeaves)
{
	std::stringstream languages;
	languages << "german_language = { color = map_german dialects = { dutch_dialect = {} low_german_dialect = {} } male_names = { name_x } }\n";
	languages << "greek_language = { dialects = { greek_language = {} koine_dialect = {} } }\n";
	languages << "swedish_language = { male_names = { name_y } }\n";
	EU5::GameDatabase database;
	database.loadLanguagesFromStream(languages);

	std::stringstream cultures;
	cultures << "hollandish = { language = dutch_dialect }\n";
	cultures << "frisian = { language = dutch_dialect }\n";
	cultures << "saxon = { language = low_german_dialect }\n";
	database.loadCulturesFromStream(cultures);

	EXPECT_EQ(3, database.getLanguageCount());
	// A dialect-bearing language is only referenceable through a dialect - the most spoken one.
	EXPECT_EQ("dutch_dialect", database.resolveLanguage("german_language"));
	// A language listing itself among its dialects is referenceable as-is (greek_language does this).
	EXPECT_EQ("greek_language", database.resolveLanguage("greek_language"));
	// Languages without dialects, dialects themselves and unknowns pass through untouched.
	EXPECT_EQ("swedish_language", database.resolveLanguage("swedish_language"));
	EXPECT_EQ("low_german_dialect", database.resolveLanguage("low_german_dialect"));
	EXPECT_EQ("martian_language", database.resolveLanguage("martian_language"));
	EXPECT_TRUE(database.resolveLanguage("").empty());
}

TEST(EU5World_GameDatabaseTests, dialectlessDialectBearersFallBackToTheFirstDeclared)
{
	std::stringstream languages;
	languages << "baltic_language = { dialects = { latvian_dialect = {} lithuanian_dialect = {} } }\n";
	EU5::GameDatabase database;
	database.loadLanguagesFromStream(languages);

	// No culture speaks either dialect, so the first declared one stands in.
	EXPECT_EQ("latvian_dialect", database.resolveLanguage("baltic_language"));
}

TEST(EU5World_GameDatabaseTests, monarchNamesResolveThroughEverySpelling)
{
	EU5::GameDatabase database;
	database.loadCharacterNames("test_files/character_names_l_english.yml");

	// The key's own name, the base localization and every language variant all lead to the same key.
	EXPECT_EQ("name_henry", database.getNameKey("Henry"));
	EXPECT_EQ("name_henry", database.getNameKey("Henrik"));
	EXPECT_EQ("name_henry", database.getNameKey("Harri"));
	EXPECT_EQ("name_henry", database.getNameKey("Henri"));
	// Punctuation and case are noise: CK3 and EU5 spell compound names differently.
	EXPECT_EQ("name_abd_al_qadir", database.getNameKey("Abd al-Qadir"));
	EXPECT_EQ("name_christopher", database.getNameKey("christopher"));
}

TEST(EU5World_GameDatabaseTests, unknownMonarchNamesResolveToNothing)
{
	EU5::GameDatabase database;
	database.loadCharacterNames("test_files/character_names_l_english.yml");

	EXPECT_TRUE(database.getNameKey("Muammar").empty());
	EXPECT_TRUE(database.getNameKey("Not a name").empty());
}

TEST(EU5World_GameDatabaseTests, missingMonarchNameFileIsSurvivable)
{
	EU5::GameDatabase database;
	database.loadCharacterNames("test_files/no_such_names.yml");

	EXPECT_EQ(0, database.getNameKeyCount());
	EXPECT_TRUE(database.getNameKey("Henry").empty());
}
