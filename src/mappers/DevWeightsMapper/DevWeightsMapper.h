#ifndef DEV_WEIGHTS_MAPPER_H
#define DEV_WEIGHTS_MAPPER_H
#include "Parser.h"

namespace mappers
{
// Tunable weights for translating CK3 county development and buildings into EU5
// location development, pop scaling and town thresholds (configurables/dev_weights.txt).
class DevWeightsMapper: commonItems::parser
{
  public:
	DevWeightsMapper() = default;
	explicit DevWeightsMapper(std::istream& theStream);
	void loadWeights(const std::filesystem::path& fileName);

	[[nodiscard]] auto getDevDivisor() const { return devDivisor; }
	[[nodiscard]] auto getMaxBonus() const { return maxBonus; }
	[[nodiscard]] auto getBuildingWeight() const { return buildingWeight; }
	[[nodiscard]] auto getUrbanDensityAllowance() const { return urbanDensityAllowance; }
	[[nodiscard]] auto getCityShareOfUrban() const { return cityShareOfUrban; }
	[[nodiscard]] auto getPopBaseFactor() const { return popBaseFactor; }
	[[nodiscard]] auto getPopDevFactor() const { return popDevFactor; }
	[[nodiscard]] auto getPopMaxFactor() const { return popMaxFactor; }
	[[nodiscard]] auto getMaaRatio() const { return maaRatio; }
	[[nodiscard]] auto getRegimentCapPerLocations() const { return regimentCapPerLocations; }

  private:
	void registerKeys();

	int devDivisor = 4;					 // CK3 development divided by this = base location bonus
	int maxBonus = 15;					 // ceiling for the per-location development bonus
	double buildingWeight = 0.2;		 // extra bonus per CK3 building in the source holding
	double urbanDensityAllowance = 1.2; // converted land's urban count as a multiple of vanilla's on that same land
	double cityShareOfUrban = 0.26;	  // fraction of urban locations that hold city rank, matching vanilla
	double popBaseFactor = 0.6;		 // pop size multiplier at zero development
	double popDevFactor = 0.03;		 // extra pop multiplier per point of CK3 development
	double popMaxFactor = 2.5;			 // pop multiplier ceiling
	double maaRatio = 0.05;				 // fraction of CK3 men-at-arms kept as EU5 standing regulars
	int regimentCapPerLocations = 60; // one extra starting regiment per this many owned locations
};
} // namespace mappers

#endif // DEV_WEIGHTS_MAPPER_H
