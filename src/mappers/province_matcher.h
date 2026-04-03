#pragma once

#include "ck3/world.h"
#include "eu5/framework.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ck3eu5::mappers {

struct ProvinceMatch
{
	std::vector<std::string> eu5_locations;
	std::string source;
};

class ProvinceMatcher
{
  public:
	explicit ProvinceMatcher(const eu5::WorldFramework& framework);

	[[nodiscard]] std::optional<ProvinceMatch> match(const ck3::County& county) const;

  private:
	const eu5::WorldFramework& framework_;
	std::map<std::string, std::string> unique_display_names_;
};

}  // namespace ck3eu5::mappers
