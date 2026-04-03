#include "common/string_utils.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace ck3eu5::common {

std::string trim(std::string_view value)
{
	size_t start = 0;
	while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
	{
		++start;
	}
	size_t end = value.size();
	while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
	{
		--end;
	}
	return std::string(value.substr(start, end - start));
}

std::string toLower(std::string_view value)
{
	std::string output(value);
	std::transform(output.begin(), output.end(), output.begin(), [](const unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return output;
}

std::string toUpper(std::string_view value)
{
	std::string output(value);
	std::transform(output.begin(), output.end(), output.begin(), [](const unsigned char c) {
		return static_cast<char>(std::toupper(c));
	});
	return output;
}

std::vector<std::string> split(std::string_view value, const char delimiter, const bool skip_empty)
{
	std::vector<std::string> result;
	std::string current;
	for (const char c: value)
	{
		if (c == delimiter)
		{
			if (!current.empty() || !skip_empty)
			{
				result.push_back(trim(current));
			}
			current.clear();
			continue;
		}
		current.push_back(c);
	}
	if (!current.empty() || !skip_empty)
	{
		result.push_back(trim(current));
	}
	return result;
}

std::string join(const std::vector<std::string>& values, std::string_view delimiter)
{
	std::ostringstream out;
	bool first = true;
	for (const auto& value: values)
	{
		if (!first)
		{
			out << delimiter;
		}
		out << value;
		first = false;
	}
	return out.str();
}

bool parseBool(std::string_view value, const bool fallback)
{
	const auto lowered = toLower(trim(value));
	if (lowered == "yes" || lowered == "true" || lowered == "1")
	{
		return true;
	}
	if (lowered == "no" || lowered == "false" || lowered == "0")
	{
		return false;
	}
	return fallback;
}

std::optional<int> parseInt(std::string_view value)
{
	try
	{
		size_t processed = 0;
		const auto trimmed = trim(value);
		const int parsed = std::stoi(trimmed, &processed);
		if (processed == trimmed.size())
		{
			return parsed;
		}
	}
	catch (...)
	{
	}
	return std::nullopt;
}

std::optional<double> parseDouble(std::string_view value)
{
	try
	{
		size_t processed = 0;
		const auto trimmed = trim(value);
		const double parsed = std::stod(trimmed, &processed);
		if (processed == trimmed.size())
		{
			return parsed;
		}
	}
	catch (...)
	{
	}
	return std::nullopt;
}

std::string stripQuotes(std::string_view value)
{
	const auto trimmed = trim(value);
	if (trimmed.size() >= 2 && ((trimmed.front() == '"' && trimmed.back() == '"') || (trimmed.front() == '\'' && trimmed.back() == '\'')))
	{
		return trimmed.substr(1, trimmed.size() - 2);
	}
	return trimmed;
}

std::string sanitizeIdentifier(std::string_view value, const char replacement)
{
	std::string output;
	output.reserve(value.size());
	for (const char c: value)
	{
		if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
		{
			output.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
		}
		else if (!output.empty() && output.back() != replacement)
		{
			output.push_back(replacement);
		}
	}
	while (!output.empty() && output.back() == replacement)
	{
		output.pop_back();
	}
	if (output.empty())
	{
		return "unnamed";
	}
	return output;
}

std::string replaceAll(std::string value, std::string_view needle, std::string_view replacement)
{
	if (needle.empty())
	{
		return value;
	}

	size_t position = 0;
	while ((position = value.find(needle, position)) != std::string::npos)
	{
		value.replace(position, needle.size(), replacement);
		position += replacement.size();
	}
	return value;
}

std::string makeSafeLocalization(std::string_view value)
{
	std::string output;
	output.reserve(value.size());
	for (const char c: value)
	{
		if (c == '"')
		{
			output += "\\\"";
		}
		else
		{
			output.push_back(c);
		}
	}
	return output;
}

}  // namespace ck3eu5::common
