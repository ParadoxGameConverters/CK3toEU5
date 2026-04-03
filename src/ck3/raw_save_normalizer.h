#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace ck3eu5::ck3 {

std::string normalizeMeltedSave(std::string_view melted_save_text);
std::string normalizeSaveFile(const std::filesystem::path& save_path);

}  // namespace ck3eu5::ck3
