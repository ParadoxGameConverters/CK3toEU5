#include "ProvinceMapper.h"
#include "CommonRegexes.h"
#include "Log.h"
#include "ParserHelpers.h"

const std::vector<std::string> mappers::ProvinceMapper::emptyLocations;
const std::vector<long long> mappers::ProvinceMapper::emptyProvinces;

namespace
{
// A single link = { ck3 = 123 ck3 = 124 eu5 = location_a eu5 = location_b }
class ProvinceMappingLink: commonItems::parser
{
  public:
	explicit ProvinceMappingLink(std::istream& theStream)
	{
		registerKeyword("ck3", [this](std::istream& stream) {
			ck3Provinces.push_back(commonItems::getLlong(stream));
		});
		registerKeyword("eu5", [this](std::istream& stream) {
			eu5Locations.push_back(commonItems::getString(stream));
		});
		registerKeyword("comment", [](std::istream& stream) {
			commonItems::ignoreItem("unused", stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::vector<long long> ck3Provinces;
	std::vector<std::string> eu5Locations;
};
} // namespace

mappers::ProvinceMapper::ProvinceMapper(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::ProvinceMapper::loadProvinceMappings(const std::filesystem::path& fileName)
{
	registerKeys();
	parseFile(fileName);
	clearRegisteredKeywords();
}

void mappers::ProvinceMapper::registerKeys()
{
	// The file is wrapped in a version block, e.g. 0.0.0 = { ... }
	registerRegex(R"(\d+\.\d+\.\d+)", [this](const std::string&, std::istream& theStream) {
		auto versionParser = commonItems::parser();
		versionParser.registerKeyword("link", [this](std::istream& stream) {
			const ProvinceMappingLink link(stream);
			if (link.ck3Provinces.empty() || link.eu5Locations.empty())
				return; // comment-only or unfinished links.
			for (const auto& province: link.ck3Provinces)
			{
				auto& locations = ck3ToEU5[province];
				locations.insert(locations.end(), link.eu5Locations.begin(), link.eu5Locations.end());
			}
			for (const auto& location: link.eu5Locations)
			{
				auto& provinces = eu5ToCK3[location];
				provinces.insert(provinces.end(), link.ck3Provinces.begin(), link.ck3Provinces.end());
			}
			++mappingCount;
		});
		versionParser.registerKeyword("triangulation_pair", [](std::istream& stream) {
			commonItems::ignoreItem("unused", stream);
		});
		versionParser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		versionParser.parseStream(theStream);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

const std::vector<std::string>& mappers::ProvinceMapper::getEU5Locations(long long ck3Province) const
{
	const auto& mapping = ck3ToEU5.find(ck3Province);
	if (mapping != ck3ToEU5.end())
		return mapping->second;
	return emptyLocations;
}

const std::vector<long long>& mappers::ProvinceMapper::getCK3Provinces(const std::string& eu5Location) const
{
	const auto& mapping = eu5ToCK3.find(eu5Location);
	if (mapping != eu5ToCK3.end())
		return mapping->second;
	return emptyProvinces;
}
