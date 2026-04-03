#include "eu5/framework.h"

namespace ck3eu5::eu5 {

const LocationDefinition* WorldFramework::getLocation(const std::string& key) const
{
	const auto it = locations.find(key);
	return it == locations.end() ? nullptr : &it->second;
}

std::vector<std::string> WorldFramework::locationsForProvince(const std::string& province_definition) const
{
	const auto it = province_to_locations.find(province_definition);
	return it == province_to_locations.end() ? std::vector<std::string>{} : it->second;
}

}  // namespace ck3eu5::eu5
