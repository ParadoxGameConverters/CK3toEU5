#pragma once

#include "config/configuration.h"

#include <filesystem>

namespace ck3eu5::config {

class ConfigurationLoader
{
  public:
	Configuration load(const std::filesystem::path& path) const;

  private:
	static std::filesystem::path resolvePath(const std::filesystem::path& config_path, const std::string& candidate);
};

}  // namespace ck3eu5::config
