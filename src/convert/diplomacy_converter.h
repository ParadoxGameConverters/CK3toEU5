#pragma once

#include "ck3/world.h"
#include "convert/country_context.h"
#include "eu5/world.h"

#include <set>
#include <string>
#include <unordered_map>

namespace ck3eu5::convert {

class DiplomacyConverter
{
  public:
	void convert(const ck3::World& ck3_world,
		 const BorderGraph& border_graph,
		 const std::set<std::string>& country_characters,
		 const std::unordered_map<std::string, std::string>& character_to_tag,
		 eu5::World& world) const;
};

}  // namespace ck3eu5::convert
