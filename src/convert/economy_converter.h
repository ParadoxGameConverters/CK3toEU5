#pragma once

#include "convert/country_context.h"
#include "eu5/framework.h"
#include "eu5/world.h"

namespace ck3eu5::convert {

class EconomyConverter
{
  public:
	void convert(const BorderGraph& border_graph, const eu5::WorldFramework& framework, eu5::World& world) const;
};

}  // namespace ck3eu5::convert
