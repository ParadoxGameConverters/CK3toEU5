#pragma once

#include "ck3/world.h"
#include "eu5/world.h"

#include <set>
#include <string>
#include <unordered_map>

namespace ck3eu5::convert {

class WarConverter
{
  public:
	void convert(const ck3::World& ck3_world,
		 const std::set<std::string>& country_characters,
		 const std::unordered_map<std::string, std::string>& character_to_tag,
		 const std::unordered_map<std::string, std::string>& county_to_primary_location,
		 eu5::World& world) const;
};

}  // namespace ck3eu5::convert
