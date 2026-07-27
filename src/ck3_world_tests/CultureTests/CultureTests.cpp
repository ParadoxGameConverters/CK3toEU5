#include "src/ck3_world/Cultures/Culture.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(CK3World_CultureTests, cultureIDLoads)
{
	std::stringstream input;
	const CK3::Culture culture(input, 42);

	EXPECT_EQ(42, culture.getID());
}

TEST(CK3World_CultureTests, cultureDefaultsToNonameDynamic)
{
	std::stringstream input;
	const CK3::Culture culture(input, 1);

	EXPECT_TRUE(culture.isDynamic());
	EXPECT_EQ("noname", culture.getName());
	EXPECT_FALSE(culture.getLocalizedName());
	EXPECT_FALSE(culture.getTemplate());
	EXPECT_TRUE(culture.getHeritage().empty());
	EXPECT_TRUE(culture.getLanguage().empty());
	EXPECT_TRUE(culture.getNameLists().empty());
	EXPECT_TRUE(culture.getTraditions().empty());
	EXPECT_TRUE(culture.getEthos().empty());
	EXPECT_FALSE(culture.getColor());
}

TEST(CK3World_CultureTests, culturePrimitivesCanBeLoaded)
{
	std::stringstream input;
	input << "culture_template=\"czech\"\n";
	input << "name=\"Bohemian\"\n";
	input << "heritage=heritage_west_slavic\n";
	input << "language=language_czech\n";
	input << "ethos=ethos_courtly\n";
	input << "traditions={ tradition_a tradition_b }\n";
	input << "name_list=name_list_czech\n";
	const CK3::Culture culture(input, 7);

	EXPECT_FALSE(culture.isDynamic());
	EXPECT_EQ("czech", culture.getName());
	EXPECT_EQ("czech", *culture.getTemplate());
	EXPECT_EQ("Bohemian", *culture.getLocalizedName());
	EXPECT_EQ("heritage_west_slavic", culture.getHeritage());
	EXPECT_EQ("language_czech", culture.getLanguage());
	EXPECT_EQ("ethos_courtly", culture.getEthos());
	EXPECT_EQ(2, culture.getTraditions().size());
	EXPECT_TRUE(culture.getNameLists().contains("czech"));
}

TEST(CK3World_CultureTests, dynamicCultureUsesLocalizedName)
{
	std::stringstream input;
	input << "name=\"Swiss\"\n";
	input << "heritage=heritage_central_germanic\n";
	input << "language=language_german\n";
	const CK3::Culture culture(input, 9);

	EXPECT_TRUE(culture.isDynamic());
	EXPECT_EQ("Swiss", culture.getName());
	EXPECT_EQ("heritage_central_germanic", culture.getHeritage());
	EXPECT_EQ("language_german", culture.getLanguage());
}

TEST(CK3World_CultureTests, cultureKeepsItsMapColor)
{
	std::stringstream input;
	input << "name=\"Swiss\"\n";
	input << "color=rgb { 224 66 44 }\n";
	const CK3::Culture culture(input, 9);

	ASSERT_TRUE(culture.getColor());
	const auto [red, green, blue] = culture.getColor()->getRgbComponents();
	EXPECT_EQ(224, red);
	EXPECT_EQ(66, green);
	EXPECT_EQ(44, blue);
}
