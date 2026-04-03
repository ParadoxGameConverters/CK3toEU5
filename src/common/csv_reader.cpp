#include "common/csv_reader.h"

#include "common/filesystem_utils.h"
#include "common/string_utils.h"

#include <sstream>
#include <stdexcept>

namespace ck3eu5::common {

std::string CsvRow::get(std::string_view column, const std::string& fallback) const
{
	const auto it = values.find(std::string(column));
	return it == values.end() ? fallback : it->second;
}

bool CsvRow::has(std::string_view column) const
{
	return values.contains(std::string(column));
}

std::vector<std::string> CsvReader::parseLine(std::string_view line)
{
	std::vector<std::string> result;
	std::string current;
	bool inside_quotes = false;

	for (size_t i = 0; i < line.size(); ++i)
	{
		const char c = line[i];
		if (c == '"')
		{
			if (inside_quotes && i + 1 < line.size() && line[i + 1] == '"')
			{
				current.push_back('"');
				++i;
				continue;
			}
			inside_quotes = !inside_quotes;
			continue;
		}
		if (c == ',' && !inside_quotes)
		{
			result.push_back(trim(current));
			current.clear();
			continue;
		}
		current.push_back(c);
	}
	result.push_back(trim(current));
	return result;
}

std::vector<CsvRow> CsvReader::readFile(const std::filesystem::path& path)
{
	const auto content = readTextFile(path);
	std::istringstream input(content);
	std::string line;
	std::vector<std::string> headers;
	std::vector<CsvRow> rows;

	while (std::getline(input, line))
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		if (trim(line).empty())
		{
			continue;
		}
		if (trim(line).starts_with('#'))
		{
			continue;
		}

		if (headers.empty())
		{
			headers = parseLine(line);
			if (!headers.empty() && headers.front().size() >= 3 &&
				 static_cast<unsigned char>(headers.front()[0]) == 0xEF &&
				 static_cast<unsigned char>(headers.front()[1]) == 0xBB &&
				 static_cast<unsigned char>(headers.front()[2]) == 0xBF)
			{
				headers.front().erase(0, 3);
			}
			for (auto& header: headers)
			{
				header = trim(stripQuotes(header));
			}
			continue;
		}

		const auto values = parseLine(line);
		CsvRow row;
		for (size_t i = 0; i < headers.size(); ++i)
		{
			row.values[headers[i]] = i < values.size() ? trim(stripQuotes(values[i])) : "";
		}
		rows.push_back(std::move(row));
	}

	if (headers.empty())
	{
		throw std::runtime_error("CSV file has no header row: " + path.string());
	}

	return rows;
}

}  // namespace ck3eu5::common
