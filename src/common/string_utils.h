#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ck3eu5::common {

std::string trim(std::string_view value);
std::string toLower(std::string_view value);
std::string toUpper(std::string_view value);
std::vector<std::string> split(std::string_view value, char delimiter, bool skip_empty = true);
std::string join(const std::vector<std::string>& values, std::string_view delimiter);
bool parseBool(std::string_view value, bool fallback = false);
std::optional<int> parseInt(std::string_view value);
std::optional<double> parseDouble(std::string_view value);
std::string stripQuotes(std::string_view value);
std::string sanitizeIdentifier(std::string_view value, char replacement = '_');
std::string replaceAll(std::string value, std::string_view needle, std::string_view replacement);
std::string makeSafeLocalization(std::string_view value);

}  // namespace ck3eu5::common
