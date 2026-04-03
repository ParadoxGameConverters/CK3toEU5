#include "common/filesystem_utils.h"

#include "lodepng.h"

#include <fstream>
#include <stdexcept>
#include <vector>

namespace ck3eu5::common {

std::string readTextFile(const std::filesystem::path& path)
{
	std::ifstream input(path, std::ios::binary);
	if (!input.is_open())
	{
		throw std::runtime_error("Could not open file: " + path.string());
	}
	return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void ensureParentDirectory(const std::filesystem::path& path)
{
	if (path.has_parent_path())
	{
		std::filesystem::create_directories(path.parent_path());
	}
}

void writeTextFile(const std::filesystem::path& path, std::string_view content, const TextEncoding encoding)
{
	ensureParentDirectory(path);
	std::ofstream output(path, std::ios::binary);
	if (!output.is_open())
	{
		throw std::runtime_error("Could not write file: " + path.string());
	}
	if (encoding == TextEncoding::Utf8Bom)
	{
		static constexpr unsigned char bom[] = {0xEF, 0xBB, 0xBF};
		output.write(reinterpret_cast<const char*>(bom), sizeof(bom));
	}
	output.write(content.data(), static_cast<std::streamsize>(content.size()));
}

void recreateDirectory(const std::filesystem::path& path)
{
	if (std::filesystem::exists(path))
	{
		std::filesystem::remove_all(path);
	}
	std::filesystem::create_directories(path);
}

void writePlaceholderThumbnailPng(const std::filesystem::path& path)
{
	constexpr unsigned width = 32;
	constexpr unsigned height = 32;
	std::vector<unsigned char> rgba(width * height * 4);
	for (unsigned y = 0; y < height; ++y)
	{
		for (unsigned x = 0; x < width; ++x)
		{
			const auto offset = static_cast<std::size_t>((y * width + x) * 4);
			rgba[offset] = static_cast<unsigned char>(48 + ((x * 5) % 96));
			rgba[offset + 1] = static_cast<unsigned char>(72 + ((y * 4) % 80));
			rgba[offset + 2] = static_cast<unsigned char>(110 + (((x + y) * 3) % 72));
			rgba[offset + 3] = 255;
		}
	}

	std::vector<unsigned char> png_bytes;
	const auto error = lodepng::encode(png_bytes, rgba, width, height);
	if (error != 0)
	{
		throw std::runtime_error("Could not encode placeholder thumbnail PNG: " +
								 std::string(lodepng_error_text(error)));
	}
	ensureParentDirectory(path);
	std::ofstream output(path, std::ios::binary);
	if (!output.is_open())
	{
		throw std::runtime_error("Could not write file: " + path.string());
	}
	output.write(reinterpret_cast<const char*>(png_bytes.data()), static_cast<std::streamsize>(png_bytes.size()));
}

}  // namespace ck3eu5::common
