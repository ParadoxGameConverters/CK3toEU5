#include "src/output/BlockParsing.h"
#include "gtest/gtest.h"
#include <sstream>

namespace
{
// Two provinces of an area, matching how EU5 nests its map data. The second is named after its own
// main location, which vanilla does throughout the map.
EU5::LocationDefinitions makeDefinitions()
{
	std::stringstream input;
	input << "africa = {\n";
	input << "\tswahili_region = {\n";
	input << "\t\tkilwa_area = {\n";
	input << "\t\t\tkilwa_province = {\n";
	input << "\t\t\t\tkilwa mikindani mitimiri\n";
	input << "\t\t\t}\n";
	input << "\t\t\tmombasa = {\n";
	input << "\t\t\t\tmombasa malindi\n";
	input << "\t\t\t}\n";
	input << "\t\t}\n";
	input << "\t}\n";
	input << "}\n";
	EU5::LocationDefinitions definitions;
	definitions.loadDefinitions(input);
	return definitions;
}
} // namespace

TEST(Output_BlockParsingTests, namedBlocksAreExtractedWithTheirBodies)
{
	const std::string text = "countries = {\n\tSWE = { capital = stockholm }\n\t# a comment\n\tDAN = { capital = copenhagen }\n}\n";
	const auto body = output::findBlockBody(text, "countries");
	ASSERT_NE(std::string::npos, body);

	const auto blocks = output::extractNamedBlocks(text, body);
	ASSERT_EQ(2u, blocks.size());
	EXPECT_EQ("SWE", blocks[0].first);
	EXPECT_EQ("SWE = { capital = stockholm }", blocks[0].second);
	EXPECT_EQ("DAN", blocks[1].first);
}

TEST(Output_BlockParsingTests, aBlockBodyComesBackWithoutItsBraces)
{
	const std::string text = "wharf = {\n\tcategory = trade\n\tlocation_potential = {\n\t\tis_port = yes\n\t}\n\tallow = { NOT = { x = 1 } }\n}\n";

	EXPECT_EQ("\n\t\tis_port = yes\n\t", output::extractBlockBody(text, "location_potential"));
	EXPECT_EQ(" NOT = { x = 1 } ", output::extractBlockBody(text, "allow"));
	EXPECT_TRUE(output::extractBlockBody(text, "country_potential").empty());
}

TEST(Output_BlockParsingTests, nestedBracesDoNotEndABlockEarly)
{
	const std::string text = "countries = {\n\tSWE = {\n\t\tcapital = stockholm\n\t\town_core = { uppsala kalmar }\n\t}\n}\n";
	const auto blocks = output::extractNamedBlocks(text, output::findBlockBody(text, "countries"));

	ASSERT_EQ(1u, blocks.size());
	EXPECT_NE(std::string::npos, blocks[0].second.find("kalmar"));
	EXPECT_EQ('}', blocks[0].second.back());
}

TEST(Output_BlockParsingTests, missingBlockYieldsNpos)
{
	EXPECT_EQ(std::string::npos, output::findBlockBody("countries = { }", "war_manager"));
}

TEST(Output_BlockParsingTests, countriesOnConvertedLandAreDetectedThroughGroups)
{
	const auto definitions = makeDefinitions();
	const std::string kilwa = "KLW = { capital = kilwa own_core = { kilwa_province } }";
	const std::string peru = "INC = { capital = cusco own_core = { cusco } }";

	EXPECT_TRUE(output::touchesConvertedLand(kilwa, {"mikindani"}, definitions));
	EXPECT_FALSE(output::touchesConvertedLand(peru, {"mikindani"}, definitions));
}

TEST(Output_BlockParsingTests, partlyConvertedCountriesKeepTheirRemainingLocations)
{
	const auto definitions = makeDefinitions();
	const std::string block = "KLW = {\n\tcapital = kilwa\n\town_control_core = { kilwa_province }\n}";

	const auto trimmed = output::trimToSurvivingLand(block, {"mikindani"}, definitions);
	ASSERT_TRUE(trimmed);
	EXPECT_NE(std::string::npos, trimmed->find("kilwa"));
	EXPECT_NE(std::string::npos, trimmed->find("mitimiri"));
	EXPECT_EQ(std::string::npos, trimmed->find("mikindani"));
	EXPECT_EQ(std::string::npos, trimmed->find("kilwa_province"));
}

TEST(Output_BlockParsingTests, untouchedCountriesComeBackVerbatim)
{
	const auto definitions = makeDefinitions();
	const std::string block = "INC = {\n\tcapital = cusco\n\town_control_core = { cusco }\n}";

	const auto trimmed = output::trimToSurvivingLand(block, {"mikindani"}, definitions);
	ASSERT_TRUE(trimmed);
	EXPECT_EQ(block, *trimmed);
}

TEST(Output_BlockParsingTests, aProvinceNamedAfterItsLocationIsReadAsTheLocation)
{
	const auto definitions = makeDefinitions();
	const std::string block = "MOM = {\n\tcapital = mombasa\n\town_control_core = { mombasa }\n}";

	// Owning the location "mombasa" says nothing about its neighbour, even though a province of
	// that name holds both.
	EXPECT_FALSE(output::touchesConvertedLand(block, {"malindi"}, definitions));

	const auto trimmed = output::trimToSurvivingLand(block, {"malindi"}, definitions);
	ASSERT_TRUE(trimmed);
	EXPECT_EQ(block, *trimmed);
}

TEST(Output_BlockParsingTests, coresOnForeignLandAreNotOwnership)
{
	const auto definitions = makeDefinitions();
	// our_cores_conquered_by_others is a claim, not a holding: a country with nothing else has no land.
	const std::string onlyClaims = "KLW = {\n\tcapital = zanzibar\n\tour_cores_conquered_by_others = { kilwa mitimiri }\n}";

	EXPECT_FALSE(output::touchesConvertedLand(onlyClaims, {"kilwa"}, definitions));
	EXPECT_FALSE(output::trimToSurvivingLand(onlyClaims, {}, definitions));
}

TEST(Output_BlockParsingTests, countriesLosingTheirCapitalOrAllLandDoNotSurvive)
{
	const auto definitions = makeDefinitions();
	const std::string lostCapital = "KLW = {\n\tcapital = kilwa\n\town_control_core = { kilwa mitimiri }\n}";
	const std::string lostEverything = "KLW = {\n\tcapital = zanzibar\n\town_control_core = { mikindani }\n}";

	EXPECT_FALSE(output::trimToSurvivingLand(lostCapital, {"kilwa"}, definitions));
	EXPECT_FALSE(output::trimToSurvivingLand(lostEverything, {"mikindani"}, definitions));
}
