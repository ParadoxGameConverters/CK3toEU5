#include "src/eu5_world/LocationDefinitions.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(EU5World_LocationDefinitionsTests, locationsDefaultToEmpty)
{
	const EU5::LocationDefinitions definitions;

	EXPECT_EQ(0, definitions.getLocationCount());
	EXPECT_FALSE(definitions.isValidLocation("stockholm"));
}

TEST(EU5World_LocationDefinitionsTests, nestedLocationsCanBeLoaded)
{
	std::stringstream input;
	input << "europe = {\n";
	input << "\twestern_europe = {\n";
	input << "\t\tscandinavian_region = {\n";
	input << "\t\t\tsvealand_area = {\n";
	input << "\t\t\t\tuppland_province = { stockholm norrtalje enkoping } # a comment\n";
	input << "\t\t\t\tsodermanland_province = { nykoping }\n";
	input << "\t\t\t}\n";
	input << "\t\t}\n";
	input << "\t}\n";
	input << "}\n";
	EU5::LocationDefinitions definitions;
	definitions.loadDefinitions(input);

	EXPECT_EQ(4, definitions.getLocationCount());
	EXPECT_TRUE(definitions.isValidLocation("stockholm"));
	EXPECT_TRUE(definitions.isValidLocation("nykoping"));
	EXPECT_FALSE(definitions.isValidLocation("uppland_province")); // group names are not locations.
	EXPECT_FALSE(definitions.isValidLocation("europe"));
}
