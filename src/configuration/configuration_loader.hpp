#ifndef CONFIGURATION_CONFIGURATIONIMPORTER_H
#define CONFIGURATION_CONFIGURATIONIMPORTER_H



#include <external/commonItems/Parser.h>

#include <filesystem>
#include <string_view>

#include "src/configuration/configuration.hpp"



namespace configuration
{

[[nodiscard]] Configuration LoadConfiguration(const std::filesystem::path& configuration_file);

}  // namespace configuration



#endif  // CONFIGURATION_CONFIGURATIONIMPORTER_H#pragma once
