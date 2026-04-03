#pragma once

#include "ck3/world.h"
#include "eu5/world.h"
#include "mappers/mapper_bundle.h"

#include <vector>

namespace ck3eu5::convert {

class PopSynthesizer
{
  public:
	std::vector<eu5::Pop> synthesize(const ck3::County& county,
		 const ck3::Character* owner,
		 const mappers::MapperBundle& mappers,
		 size_t split_count) const;
};

}  // namespace ck3eu5::convert
