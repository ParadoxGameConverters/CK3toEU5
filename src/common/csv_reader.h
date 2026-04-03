#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ck3eu5::common {

class CsvRow
{
  public:
	std::string get(std::string_view column, const std::string& fallback = "") const;
	bool has(std::string_view column) const;

	std::unordered_map<std::string, std::string> values;
};

class CsvReader
{
  public:
	static std::vector<CsvRow> readFile(const std::filesystem::path& path);

  private:
	static std::vector<std::string> parseLine(std::string_view line);
};

}  // namespace ck3eu5::common
