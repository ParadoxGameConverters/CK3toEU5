#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace ck3eu5::common {

enum class TextEncoding
{
	Utf8NoBom,
	Utf8Bom
};

std::string readTextFile(const std::filesystem::path& path);
void writeTextFile(const std::filesystem::path& path, std::string_view content, TextEncoding encoding = TextEncoding::Utf8NoBom);
void ensureParentDirectory(const std::filesystem::path& path);
void recreateDirectory(const std::filesystem::path& path);
void writePlaceholderThumbnailPng(const std::filesystem::path& path);

}  // namespace ck3eu5::common
