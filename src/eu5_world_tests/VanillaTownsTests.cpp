#include "src/eu5_world/VanillaTowns.h"
#include "gtest/gtest.h"
#include <sstream>

namespace
{
EU5::VanillaTowns loadTowns()
{
	std::stringstream input;
	input << "locations={\n";
	input << "\tstockholm = { rank = city town_setup = scandinavian_city_port }\n";
	input << "\tuppsala = { rank = town town_setup = scandinavian_town }\n";
	input << "\tconstantinople = { rank = megalopolis town_setup = byzantine_city }\n";
	input << "\tkalmar = { some_other_data = yes }\n";
	input << "}\n";
	input << "building_manager = {\n";
	input << "\tcastle = { tag = SWE level = 1 location = visby rank = city }\n";
	input << "}\n";
	EU5::VanillaTowns towns;
	towns.loadTowns(input);
	return towns;
}
} // namespace

TEST(EU5World_VanillaTownsTests, rankedLocationsAreTownsAndUnrankedOnesAreNot)
{
	const auto towns = loadTowns();

	EXPECT_EQ(3, towns.getTownCount());
	EXPECT_TRUE(towns.isUrban("stockholm"));
	EXPECT_TRUE(towns.isUrban("uppsala"));
	// Present in the locations block but carrying no rank, so not a town.
	EXPECT_FALSE(towns.isUrban("kalmar"));
	EXPECT_FALSE(towns.isUrban("nowhere"));
	// Outside the locations block entirely.
	EXPECT_FALSE(towns.isUrban("visby"));
}

TEST(EU5World_VanillaTownsTests, cityRankCoversMegalopolisButNotTown)
{
	const auto towns = loadTowns();

	EXPECT_TRUE(towns.isCity("stockholm"));
	EXPECT_TRUE(towns.isCity("constantinople"));
	EXPECT_FALSE(towns.isCity("uppsala"));
	EXPECT_FALSE(towns.isCity("nowhere"));
}

TEST(EU5World_VanillaTownsTests, setupsComeAlongForBuildingConversion)
{
	const auto towns = loadTowns();

	EXPECT_EQ("scandinavian_town", towns.getTowns().at("uppsala").setup);
	EXPECT_EQ("city", towns.getTowns().at("stockholm").rank);
}
