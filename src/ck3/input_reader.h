#pragma once

#include "config/configuration.h"

#include <string>

namespace ck3eu5::ck3 {

class InputReader
{
  public:
	std::string read(const config::Configuration& configuration) const;
};

}  // namespace ck3eu5::ck3
