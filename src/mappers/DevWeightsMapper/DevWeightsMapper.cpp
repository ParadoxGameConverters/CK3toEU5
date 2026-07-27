#include "DevWeightsMapper.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"
#include <algorithm>

mappers::DevWeightsMapper::DevWeightsMapper(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::DevWeightsMapper::loadWeights(const std::filesystem::path& fileName)
{
	registerKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::DevWeightsMapper::registerKeys()
{
	registerKeyword("dev_divisor", [this](std::istream& stream) {
		devDivisor = std::max(1, commonItems::getInt(stream));
	});
	registerKeyword("max_bonus", [this](std::istream& stream) {
		maxBonus = commonItems::getInt(stream);
	});
	registerKeyword("building_weight", [this](std::istream& stream) {
		buildingWeight = commonItems::getDouble(stream);
	});
	registerKeyword("urban_density_allowance", [this](std::istream& stream) {
		urbanDensityAllowance = std::max(1.0, commonItems::getDouble(stream));
	});
	registerKeyword("city_share_of_urban", [this](std::istream& stream) {
		cityShareOfUrban = std::clamp(commonItems::getDouble(stream), 0.0, 1.0);
	});
	registerKeyword("pop_base_factor", [this](std::istream& stream) {
		popBaseFactor = commonItems::getDouble(stream);
	});
	registerKeyword("pop_dev_factor", [this](std::istream& stream) {
		popDevFactor = commonItems::getDouble(stream);
	});
	registerKeyword("pop_max_factor", [this](std::istream& stream) {
		popMaxFactor = commonItems::getDouble(stream);
	});
	registerKeyword("maa_ratio", [this](std::istream& stream) {
		maaRatio = commonItems::getDouble(stream);
	});
	registerKeyword("regiment_cap_per_locations", [this](std::istream& stream) {
		regimentCapPerLocations = std::max(1, commonItems::getInt(stream));
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}
