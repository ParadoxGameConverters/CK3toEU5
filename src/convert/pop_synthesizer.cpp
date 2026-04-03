#include "convert/pop_synthesizer.h"

#include <algorithm>

namespace ck3eu5::convert {

std::vector<eu5::Pop> PopSynthesizer::synthesize(const ck3::County& county,
	 const ck3::Character* owner,
	 const mappers::MapperBundle& mappers,
	 const size_t split_count) const
{
	const double divisor = std::max<size_t>(split_count, 1);
	const auto mass_culture = mappers.mapCulture(county.culture);
	const auto mass_religion = mappers.mapReligion(county.faith);
	const auto elite_culture = owner && !owner->culture.empty() ? mappers.mapCulture(owner->culture) : mass_culture;
	const auto elite_religion = owner && !owner->faith.empty() ? mappers.mapReligion(owner->faith) : mass_religion;

	const int castles = static_cast<int>(std::count(county.holdings.begin(), county.holdings.end(), "castle"));
	const int cities = static_cast<int>(std::count(county.holdings.begin(), county.holdings.end(), "city"));
	const int temples = static_cast<int>(std::count(county.holdings.begin(), county.holdings.end(), "temple"));
	const bool tribal = county.government == "tribal" || std::count(county.holdings.begin(), county.holdings.end(), "tribe") > 0;

	const double development = std::max(1, county.development);

	auto shared = [divisor](const double value) {
		return value / divisor;
	};

	std::vector<eu5::Pop> pops;
	pops.push_back(
		 eu5::Pop{.type = "nobles", .culture = elite_culture, .religion = elite_religion, .size = shared(0.2 + castles * 0.7 + development * 0.05)});
	pops.push_back(
		 eu5::Pop{.type = "clergy", .culture = mass_culture, .religion = elite_religion, .size = shared(0.15 + temples * 0.5 + development * 0.03)});
	pops.push_back(
		 eu5::Pop{.type = "burghers", .culture = mass_culture, .religion = mass_religion, .size = shared(cities * 0.9 + development * 0.08)});
	pops.push_back(
		 eu5::Pop{.type = "laborers", .culture = mass_culture, .religion = mass_religion, .size = shared(0.5 + development * 0.7 + cities * 0.4)});
	pops.push_back(
		 eu5::Pop{.type = "soldiers", .culture = elite_culture, .religion = elite_religion, .size = shared(0.2 + castles * 0.25 + development * 0.04)});
	if (tribal)
	{
		pops.push_back(
			 eu5::Pop{.type = "tribesmen", .culture = mass_culture, .religion = mass_religion, .size = shared(1.0 + development * 1.2)});
	}
	else
	{
		pops.push_back(
			 eu5::Pop{.type = "peasants", .culture = mass_culture, .religion = mass_religion, .size = shared(1.0 + development * 1.6 + castles * 0.4 + temples * 0.2)});
	}

	for (auto& pop: pops)
	{
		if (pop.size < 0.001)
		{
			pop.size = 0.001;
		}
	}
	return pops;
}

}  // namespace ck3eu5::convert
