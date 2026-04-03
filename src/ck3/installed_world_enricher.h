#pragma once

#include "ck3/installed_titles.h"
#include "ck3/world.h"

namespace ck3eu5::ck3 {

class InstalledWorldEnricher
{
  public:
	void enrich(World& world, const InstalledTitles& installed_data) const;
};

}  // namespace ck3eu5::ck3
