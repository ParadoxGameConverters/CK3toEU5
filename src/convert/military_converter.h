#pragma once

#include "ck3/world.h"
#include "convert/country_context.h"
#include "eu5/framework.h"
#include "eu5/world.h"

namespace ck3eu5::convert {

class MilitaryConverter
{
  public:
	void convert(const ck3::World& ck3_world,
		 const BorderGraph& border_graph,
		 const eu5::WorldFramework& framework,
		 eu5::World& world) const;
};

}  // namespace ck3eu5::convert
