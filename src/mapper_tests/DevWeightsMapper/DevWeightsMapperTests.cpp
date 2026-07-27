#include "src/mappers/DevWeightsMapper/DevWeightsMapper.h"
#include "gtest/gtest.h"
#include <sstream>

TEST(Mappers_DevWeightsMapperTests, defaultsHoldWithoutInput)
{
	std::stringstream input;
	const mappers::DevWeightsMapper mapper(input);

	EXPECT_EQ(4, mapper.getDevDivisor());
	EXPECT_EQ(15, mapper.getMaxBonus());
	EXPECT_DOUBLE_EQ(0.2, mapper.getBuildingWeight());
	EXPECT_DOUBLE_EQ(1.2, mapper.getUrbanDensityAllowance());
	EXPECT_DOUBLE_EQ(0.26, mapper.getCityShareOfUrban());
	EXPECT_DOUBLE_EQ(0.6, mapper.getPopBaseFactor());
	EXPECT_DOUBLE_EQ(0.03, mapper.getPopDevFactor());
	EXPECT_DOUBLE_EQ(2.5, mapper.getPopMaxFactor());
	EXPECT_DOUBLE_EQ(0.05, mapper.getMaaRatio());
	EXPECT_EQ(60, mapper.getRegimentCapPerLocations());
}

TEST(Mappers_DevWeightsMapperTests, weightsCanBeOverridden)
{
	std::stringstream input;
	input << "dev_divisor = 8\n";
	input << "max_bonus = 20\n";
	input << "building_weight = 0.5\n";
	input << "urban_density_allowance = 1.5\n";
	input << "city_share_of_urban = 0.4\n";
	input << "pop_base_factor = 0.8\n";
	input << "pop_dev_factor = 0.05\n";
	input << "pop_max_factor = 3.0\n";
	input << "maa_ratio = 0.5\n";
	input << "regiment_cap_per_locations = 10\n";
	const mappers::DevWeightsMapper mapper(input);

	EXPECT_EQ(8, mapper.getDevDivisor());
	EXPECT_EQ(20, mapper.getMaxBonus());
	EXPECT_DOUBLE_EQ(0.5, mapper.getBuildingWeight());
	EXPECT_DOUBLE_EQ(1.5, mapper.getUrbanDensityAllowance());
	EXPECT_DOUBLE_EQ(0.4, mapper.getCityShareOfUrban());
	EXPECT_DOUBLE_EQ(0.8, mapper.getPopBaseFactor());
	EXPECT_DOUBLE_EQ(0.05, mapper.getPopDevFactor());
	EXPECT_DOUBLE_EQ(3.0, mapper.getPopMaxFactor());
	EXPECT_DOUBLE_EQ(0.5, mapper.getMaaRatio());
	EXPECT_EQ(10, mapper.getRegimentCapPerLocations());
}

TEST(Mappers_DevWeightsMapperTests, devDivisorNeverDropsBelowOne)
{
	std::stringstream input;
	input << "dev_divisor = 0\n";
	const mappers::DevWeightsMapper mapper(input);

	EXPECT_EQ(1, mapper.getDevDivisor());
}

TEST(Mappers_DevWeightsMapperTests, urbanQuotaStaysWithinSaneBounds)
{
	std::stringstream input;
	// Below 1.0 would demote vanilla's own towns, and a share above 1.0 makes no sense.
	input << "urban_density_allowance = 0.5\n";
	input << "city_share_of_urban = 1.7\n";
	const mappers::DevWeightsMapper mapper(input);

	EXPECT_DOUBLE_EQ(1.0, mapper.getUrbanDensityAllowance());
	EXPECT_DOUBLE_EQ(1.0, mapper.getCityShareOfUrban());
}
