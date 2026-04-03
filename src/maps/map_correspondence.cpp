#include "maps/map_correspondence.h"

#include "common/csv_reader.h"
#include "common/filesystem_utils.h"
#include "common/string_utils.h"
#include "lodepng.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ck3eu5::maps {
namespace fs = std::filesystem;

namespace {

struct RgbImage
{
	unsigned width = 0;
	unsigned height = 0;
	std::vector<unsigned char> pixels;

	[[nodiscard]] uint32_t colorAt(const unsigned x, const unsigned y) const
	{
		const auto index = (static_cast<size_t>(y) * width + x) * 3;
		return (static_cast<uint32_t>(pixels[index]) << 16) | (static_cast<uint32_t>(pixels[index + 1]) << 8) |
				 static_cast<uint32_t>(pixels[index + 2]);
	}
};

RgbImage loadRgbPng(const fs::path& path)
{
	std::vector<unsigned char> pixels;
	unsigned width = 0;
	unsigned height = 0;
	const auto error = lodepng::decode(pixels, width, height, path.string(), LCT_RGB, 8);
	if (error != 0)
	{
		throw std::runtime_error("Failed to decode PNG " + path.string() + ": " + lodepng_error_text(error));
	}
	return {.width = width, .height = height, .pixels = std::move(pixels)};
}

struct RasterStats
{
	uint64_t pixel_count = 0;
	uint64_t sum_x = 0;
	uint64_t sum_y = 0;
	unsigned min_x = std::numeric_limits<unsigned>::max();
	unsigned min_y = std::numeric_limits<unsigned>::max();
	unsigned max_x = 0;
	unsigned max_y = 0;

	void addPixel(const unsigned x, const unsigned y)
	{
		++pixel_count;
		sum_x += x;
		sum_y += y;
		min_x = std::min(min_x, x);
		min_y = std::min(min_y, y);
		max_x = std::max(max_x, x);
		max_y = std::max(max_y, y);
	}

	void merge(const RasterStats& other)
	{
		if (other.pixel_count == 0)
		{
			return;
		}
		if (pixel_count == 0)
		{
			*this = other;
			return;
		}

		pixel_count += other.pixel_count;
		sum_x += other.sum_x;
		sum_y += other.sum_y;
		min_x = std::min(min_x, other.min_x);
		min_y = std::min(min_y, other.min_y);
		max_x = std::max(max_x, other.max_x);
		max_y = std::max(max_y, other.max_y);
	}

	[[nodiscard]] double centroidX(const unsigned image_width) const
	{
		if (pixel_count == 0 || image_width == 0)
		{
			return 0.0;
		}
		return (static_cast<double>(sum_x) / static_cast<double>(pixel_count)) / static_cast<double>(image_width);
	}

	[[nodiscard]] double centroidY(const unsigned image_height) const
	{
		if (pixel_count == 0 || image_height == 0)
		{
			return 0.0;
		}
		return (static_cast<double>(sum_y) / static_cast<double>(pixel_count)) / static_cast<double>(image_height);
	}
};

struct VoteCounter
{
	std::vector<std::pair<int, uint64_t>> counts;

	void add(const int id, const uint64_t amount = 1)
	{
		for (auto& [existing_id, count]: counts)
		{
			if (existing_id == id)
			{
				count += amount;
				return;
			}
		}
		counts.emplace_back(id, amount);
	}

	void merge(const VoteCounter& other)
	{
		for (const auto& [id, count]: other.counts)
		{
			add(id, count);
		}
	}

	[[nodiscard]] uint64_t total() const
	{
		uint64_t total_votes = 0;
		for (const auto& [_, count]: counts)
		{
			total_votes += count;
		}
		return total_votes;
	}
};

std::string csvEscape(std::string_view value)
{
	std::string escaped = "\"";
	for (const char character: value)
	{
		if (character == '"')
		{
			escaped += "\"\"";
		}
		else
		{
			escaped.push_back(character);
		}
	}
	escaped.push_back('"');
	return escaped;
}

std::string joinInts(const std::vector<int>& values)
{
	std::vector<std::string> as_strings;
	as_strings.reserve(values.size());
	for (const auto value: values)
	{
		as_strings.push_back(std::to_string(value));
	}
	return common::join(as_strings, "|");
}

std::string stripTitlePrefix(std::string value)
{
	if (value.size() > 2 && value[1] == '_')
	{
		value.erase(0, 2);
	}
	return value;
}

std::string stripProvinceSuffix(std::string value)
{
	constexpr std::string_view suffix = "_province";
	if (value.size() > suffix.size() && value.ends_with(suffix))
	{
		value.erase(value.size() - suffix.size());
	}
	return value;
}

std::string stripTrailingDigits(std::string value)
{
	while (!value.empty() && std::isdigit(static_cast<unsigned char>(value.back())))
	{
		value.pop_back();
	}
	return value;
}

std::vector<fs::path> listFiles(const fs::path& directory, const std::string_view extension)
{
	std::vector<fs::path> paths;
	if (!fs::exists(directory))
	{
		return paths;
	}

	for (const auto& entry: fs::recursive_directory_iterator(directory))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}
		if (!extension.empty() && common::toLower(entry.path().extension().string()) != common::toLower(extension))
		{
			continue;
		}
		paths.push_back(entry.path());
	}

	std::sort(paths.begin(), paths.end());
	return paths;
}

struct Ck3ProvinceDefinition
{
	int id = 0;
	uint32_t color = 0;
	std::string province_token;
	std::string barony_key;
};

struct Ck3ProvinceAccumulator
{
	Ck3ProvinceDefinition definition;
	RasterStats stats;
	VoteCounter eu5_votes;
};

struct Eu5ProvinceGroup
{
	std::string province_definition;
	std::vector<std::string> locations;
	std::vector<std::string> display_names;
	RasterStats stats;
	bool likely_water = false;
};

struct ControlPoint
{
	double source_x = 0.0;
	double source_y = 0.0;
	double target_x = 0.0;
	double target_y = 0.0;
};

struct AffineTransform
{
	double ax = 1.0;
	double bx = 0.0;
	double cx = 0.0;
	double ay = 0.0;
	double by = 1.0;
	double cy = 0.0;

	[[nodiscard]] std::pair<double, double> apply(const double x, const double y) const
	{
		return {ax * x + bx * y + cx, ay * x + by * y + cy};
	}
};

std::vector<Ck3ProvinceDefinition> loadCk3ProvinceDefinitions(const fs::path& path)
{
	std::vector<Ck3ProvinceDefinition> definitions;
	std::istringstream input(common::readTextFile(path));
	std::string line;
	while (std::getline(input, line))
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		const auto fields = common::split(line, ';', false);
		if (fields.size() < 5)
		{
			continue;
		}

		const auto id = common::parseInt(fields[0]).value_or(0);
		if (id <= 0)
		{
			continue;
		}

		const auto red = common::parseInt(fields[1]).value_or(0);
		const auto green = common::parseInt(fields[2]).value_or(0);
		const auto blue = common::parseInt(fields[3]).value_or(0);
		const auto token = common::trim(fields[4]);

		Ck3ProvinceDefinition definition;
		definition.id = id;
		definition.color = (static_cast<uint32_t>(red) << 16) | (static_cast<uint32_t>(green) << 8) | static_cast<uint32_t>(blue);
		definition.province_token = token;
		definition.barony_key = "b_" + common::sanitizeIdentifier(token);
		definitions.push_back(std::move(definition));
	}
	return definitions;
}

void pushUnique(std::vector<std::string>& values, const std::string& value)
{
	if (value.empty() || std::find(values.begin(), values.end(), value) != values.end())
	{
		return;
	}
	values.push_back(value);
}

std::map<std::string, Eu5ProvinceGroup> loadEu5ProvinceGroupsFromFramework(const fs::path& path)
{
	std::map<std::string, Eu5ProvinceGroup> groups;
	for (const auto& row: common::CsvReader::readFile(path))
	{
		const auto location_key = row.get("location_key");
		const auto province_definition = row.get("province_definition");
		if (location_key.empty() || province_definition.empty())
		{
			continue;
		}
		auto& group = groups[province_definition];
		group.province_definition = province_definition;
		pushUnique(group.locations, location_key);
		pushUnique(group.display_names, row.get("display_name"));
	}
	return groups;
}

std::string readTextFileStripBom(const fs::path& path)
{
	auto text = common::readTextFile(path);
	if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF && static_cast<unsigned char>(text[1]) == 0xBB &&
		 static_cast<unsigned char>(text[2]) == 0xBF)
	{
		text.erase(0, 3);
	}
	return text;
}

std::unordered_map<uint32_t, std::string> loadEu5NamedLocationColors(const fs::path& directory)
{
	std::unordered_map<uint32_t, std::string> colors;
	for (const auto& path: listFiles(directory, ".txt"))
	{
		std::istringstream input(readTextFileStripBom(path));
		std::string line;
		while (std::getline(input, line))
		{
			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}
			const auto trimmed = common::trim(line);
			if (trimmed.empty() || trimmed.starts_with('#'))
			{
				continue;
			}
			const auto delimiter = trimmed.find('=');
			if (delimiter == std::string::npos)
			{
				continue;
			}
			const auto key = common::trim(trimmed.substr(0, delimiter));
			const auto value = common::trim(trimmed.substr(delimiter + 1));
			if (key.empty() || value.size() != 6)
			{
				continue;
			}
			try
			{
				const auto color = static_cast<uint32_t>(std::stoul(value, nullptr, 16));
				colors[color] = key;
			}
			catch (...)
			{
			}
		}
	}
	return colors;
}

double distance(const double ax, const double ay, const double bx, const double by)
{
	const auto dx = ax - bx;
	const auto dy = ay - by;
	return std::sqrt(dx * dx + dy * dy);
}

size_t boundedLevenshteinDistance(const std::string& lhs, const std::string& rhs, const size_t limit)
{
	if (lhs == rhs)
	{
		return 0;
	}
	if (lhs.empty())
	{
		return rhs.size();
	}
	if (rhs.empty())
	{
		return lhs.size();
	}
	if (lhs.size() > rhs.size() + limit || rhs.size() > lhs.size() + limit)
	{
		return limit + 1;
	}

	std::vector<size_t> previous(rhs.size() + 1);
	std::vector<size_t> current(rhs.size() + 1);
	for (size_t index = 0; index <= rhs.size(); ++index)
	{
		previous[index] = index;
	}

	for (size_t lhs_index = 0; lhs_index < lhs.size(); ++lhs_index)
	{
		current[0] = lhs_index + 1;
		size_t row_minimum = current[0];
		for (size_t rhs_index = 0; rhs_index < rhs.size(); ++rhs_index)
		{
			const auto substitution_cost = lhs[lhs_index] == rhs[rhs_index] ? 0U : 1U;
			current[rhs_index + 1] = std::min({previous[rhs_index + 1] + 1,
				 current[rhs_index] + 1,
				 previous[rhs_index] + substitution_cost});
			row_minimum = std::min(row_minimum, current[rhs_index + 1]);
		}
		if (row_minimum > limit)
		{
			return limit + 1;
		}
		std::swap(previous, current);
	}

	return previous.back();
}

std::map<std::string, std::vector<std::string>> loadControlMappings(const fs::path& path)
{
	std::map<std::string, std::vector<std::string>> mappings;
	for (const auto& row: common::CsvReader::readFile(path))
	{
		const auto county_key = row.get("ck3_county");
		if (county_key.empty())
		{
			continue;
		}
		auto locations = common::split(row.get("eu5_locations"), '|');
		locations.erase(std::remove_if(locations.begin(), locations.end(), [](const std::string& location) {
			return location.empty();
		}), locations.end());
		if (!locations.empty())
		{
			mappings[county_key] = std::move(locations);
		}
	}
	return mappings;
}

struct ControlMappingMetadata
{
	bool allow_as_anchor = true;
};

fs::path inferControlMappingReportPath(const fs::path& control_province_mappings_path)
{
	if (control_province_mappings_path.filename() == "province_mappings.csv")
	{
		return control_province_mappings_path.parent_path() / "province_mappings_report.csv";
	}
	return control_province_mappings_path.parent_path() /
			 (control_province_mappings_path.stem().string() + "_report.csv");
}

std::map<std::string, ControlMappingMetadata> loadControlMappingMetadata(const fs::path& control_province_mappings_path)
{
	const auto report_path = inferControlMappingReportPath(control_province_mappings_path);
	if (!fs::exists(report_path))
	{
		return {};
	}

	std::map<std::string, ControlMappingMetadata> metadata;
	for (const auto& row: common::CsvReader::readFile(report_path))
	{
		const auto county_key = row.get("ck3_county");
		if (county_key.empty())
		{
			continue;
		}

		const auto eu5_locations = row.get("eu5_locations");
		const auto match_sources = row.get("match_sources");
		const auto has_county_level_exact_source =
				match_sources.find("county_exact_location_key") != std::string::npos ||
				match_sources.find("county_exact_province_definition") != std::string::npos ||
				match_sources.find("existing_manual") != std::string::npos;
		metadata[county_key] = {.allow_as_anchor = !eu5_locations.empty() && has_county_level_exact_source &&
							match_sources.find("display_name") == std::string::npos};
	}

	return metadata;
}

bool isLikelyWaterProvince(std::string_view province_definition)
{
	const auto lower = common::toLower(province_definition);
	return lower.find("sea") != std::string::npos || lower.find("strait") != std::string::npos ||
			 lower.find("ocean") != std::string::npos || lower.find("channel") != std::string::npos;
}

std::optional<int> resolveControlGroupId(const std::vector<std::string>& locations,
	 const std::map<std::string, int>& eu5_location_to_group_id,
	 const std::vector<Eu5ProvinceGroup>& eu5_groups)
{
	std::optional<int> group_id;
	for (const auto& location: locations)
	{
		const auto location_it = eu5_location_to_group_id.find(location);
		if (location_it == eu5_location_to_group_id.end())
		{
			return std::nullopt;
		}
		const auto candidate_group_id = location_it->second;
		if (candidate_group_id < 0 || static_cast<size_t>(candidate_group_id) >= eu5_groups.size())
		{
			return std::nullopt;
		}
		if (eu5_groups[static_cast<size_t>(candidate_group_id)].likely_water)
		{
			return std::nullopt;
		}
		if (!group_id.has_value())
		{
			group_id = candidate_group_id;
			continue;
		}
		if (*group_id != candidate_group_id)
		{
			return std::nullopt;
		}
	}

	return group_id;
}

std::array<double, 3> solveAffineAxis(const std::vector<ControlPoint>& controls, const bool solve_x)
{
	double s_xx = 0.0;
	double s_xy = 0.0;
	double s_yy = 0.0;
	double s_x = 0.0;
	double s_y = 0.0;
	double s_tx = 0.0;
	double s_x_tx = 0.0;
	double s_y_tx = 0.0;

	for (const auto& control: controls)
	{
		const auto target = solve_x ? control.target_x : control.target_y;
		s_xx += control.source_x * control.source_x;
		s_xy += control.source_x * control.source_y;
		s_yy += control.source_y * control.source_y;
		s_x += control.source_x;
		s_y += control.source_y;
		s_tx += target;
		s_x_tx += control.source_x * target;
		s_y_tx += control.source_y * target;
	}

	double matrix[3][4] = {
		 {s_xx, s_xy, s_x, s_x_tx},
		 {s_xy, s_yy, s_y, s_y_tx},
		 {s_x, s_y, static_cast<double>(controls.size()), s_tx}};

	for (int pivot = 0; pivot < 3; ++pivot)
	{
		int best_row = pivot;
		for (int row = pivot + 1; row < 3; ++row)
		{
			if (std::fabs(matrix[row][pivot]) > std::fabs(matrix[best_row][pivot]))
			{
				best_row = row;
			}
		}

		if (std::fabs(matrix[best_row][pivot]) < 1e-12)
		{
			return solve_x ? std::array<double, 3>{1.0, 0.0, 0.0} : std::array<double, 3>{0.0, 1.0, 0.0};
		}

		if (best_row != pivot)
		{
			for (int column = pivot; column < 4; ++column)
			{
				std::swap(matrix[pivot][column], matrix[best_row][column]);
			}
		}

		const auto divisor = matrix[pivot][pivot];
		for (int column = pivot; column < 4; ++column)
		{
			matrix[pivot][column] /= divisor;
		}

		for (int row = 0; row < 3; ++row)
		{
			if (row == pivot)
			{
				continue;
			}
			const auto factor = matrix[row][pivot];
			for (int column = pivot; column < 4; ++column)
			{
				matrix[row][column] -= factor * matrix[pivot][column];
			}
		}
	}

	return {matrix[0][3], matrix[1][3], matrix[2][3]};
}

AffineTransform fitAffineTransform(const std::vector<ControlPoint>& controls)
{
	if (controls.size() < 3)
	{
		return {};
	}

	const auto x_axis = solveAffineAxis(controls, true);
	const auto y_axis = solveAffineAxis(controls, false);
	return {.ax = x_axis[0], .bx = x_axis[1], .cx = x_axis[2], .ay = y_axis[0], .by = y_axis[1], .cy = y_axis[2]};
}

double controlResidual(const ControlPoint& control, const AffineTransform& transform)
{
	const auto [mapped_x, mapped_y] = transform.apply(control.source_x, control.source_y);
	return distance(mapped_x, mapped_y, control.target_x, control.target_y);
}

struct TransformFitResult
{
	AffineTransform transform;
	size_t input_controls = 0;
	size_t inlier_controls = 0;
	double rms_error = 0.0;
	double max_error = 0.0;
};

TransformFitResult fitRobustAffineTransform(const std::vector<ControlPoint>& controls)
{
	TransformFitResult result;
	result.input_controls = controls.size();
	if (controls.size() < 3)
	{
		result.inlier_controls = controls.size();
		return result;
	}

	std::vector<ControlPoint> working_controls = controls;
	for (int iteration = 0; iteration < 4 && working_controls.size() >= 3; ++iteration)
	{
		const auto transform = fitAffineTransform(working_controls);

		struct ResidualPoint
		{
			ControlPoint control;
			double residual = 0.0;
		};

		std::vector<ResidualPoint> residuals;
		residuals.reserve(working_controls.size());
		for (const auto& control: working_controls)
		{
			residuals.push_back({.control = control, .residual = controlResidual(control, transform)});
		}

		std::vector<double> sorted_residuals;
		sorted_residuals.reserve(residuals.size());
		for (const auto& residual: residuals)
		{
			sorted_residuals.push_back(residual.residual);
		}
		std::sort(sorted_residuals.begin(), sorted_residuals.end());
		const auto keep_index = std::min(sorted_residuals.size() - 1, (sorted_residuals.size() * 4) / 5);
		const auto threshold = std::clamp(sorted_residuals[keep_index], 0.01, 0.04);

		std::vector<ControlPoint> next_controls;
		next_controls.reserve(residuals.size());
		for (const auto& residual: residuals)
		{
			if (residual.residual <= threshold)
			{
				next_controls.push_back(residual.control);
			}
		}

		if (next_controls.size() < 3 || next_controls.size() == working_controls.size())
		{
			break;
		}

		working_controls = std::move(next_controls);
	}

	result.transform = fitAffineTransform(working_controls);
	result.inlier_controls = working_controls.size();

	double squared_error_sum = 0.0;
	for (const auto& control: working_controls)
	{
		const auto residual = controlResidual(control, result.transform);
		squared_error_sum += residual * residual;
		result.max_error = std::max(result.max_error, residual);
	}
	if (!working_controls.empty())
	{
		result.rms_error = std::sqrt(squared_error_sum / static_cast<double>(working_controls.size()));
	}

	return result;
}

std::vector<std::string> buildCountyNameVariants(const CountyCorrespondence& county)
{
	std::vector<std::string> variants;
	std::set<std::string> seen;

	const auto push_variant = [&](const std::string& raw_value) {
		const auto normalized = common::sanitizeIdentifier(stripTrailingDigits(raw_value));
		if (!normalized.empty() && seen.insert(normalized).second)
		{
			variants.push_back(normalized);
		}
	};

	push_variant(stripTitlePrefix(county.ck3_county));
	push_variant(county.display_name);
	for (const auto& barony_key: county.barony_keys)
	{
		push_variant(stripTitlePrefix(barony_key));
	}
	for (const auto& barony_display_name: county.barony_display_names)
	{
		push_variant(barony_display_name);
	}

	return variants;
}

std::vector<std::string> buildCandidateNameVariants(const CorrespondenceCandidate& candidate)
{
	std::vector<std::string> variants;
	std::set<std::string> seen;

	const auto push_variant = [&](const std::string& raw_value) {
		const auto normalized = common::sanitizeIdentifier(stripTrailingDigits(stripProvinceSuffix(raw_value)));
		if (!normalized.empty() && seen.insert(normalized).second)
		{
			variants.push_back(normalized);
		}
	};

	push_variant(candidate.eu5_province_definition);
	for (const auto& location: candidate.eu5_locations)
	{
		push_variant(location);
	}
	for (const auto& display_name: candidate.eu5_display_names)
	{
		push_variant(display_name);
	}
	return variants;
}

double calculateNameAffinity(const std::vector<std::string>& county_variants, const std::vector<std::string>& candidate_variants)
{
	if (county_variants.empty() || candidate_variants.empty())
	{
		return 0.0;
	}

	double best_affinity = 0.0;
	for (const auto& county_variant: county_variants)
	{
		if (county_variant.size() < 4)
		{
			continue;
		}
		for (const auto& candidate_variant: candidate_variants)
		{
			if (candidate_variant.size() < 4)
			{
				continue;
			}
			if (county_variant == candidate_variant)
			{
				return 1.0;
			}

			if ((county_variant.find(candidate_variant) != std::string::npos ||
					 candidate_variant.find(county_variant) != std::string::npos) &&
				 std::min(county_variant.size(), candidate_variant.size()) >= 6)
			{
				best_affinity = std::max(best_affinity, 0.7);
			}

			if (const auto edit_distance = boundedLevenshteinDistance(county_variant, candidate_variant, 1); edit_distance <= 1)
			{
				best_affinity = std::max(best_affinity, 0.85);
			}
			else
			{
				const auto broader_distance = boundedLevenshteinDistance(county_variant, candidate_variant, 3);
				if (broader_distance <= 2)
				{
					best_affinity = std::max(best_affinity, 0.75);
				}
				else if (broader_distance <= 3)
				{
					best_affinity = std::max(best_affinity, 0.65);
				}
			}
		}
	}

	return best_affinity;
}

double candidateNameAffinity(const CountyCorrespondence& county, const CorrespondenceCandidate& candidate)
{
	return calculateNameAffinity(buildCountyNameVariants(county), buildCandidateNameVariants(candidate));
}

double calculateStrongNameSignal(const std::vector<std::string>& county_variants, const std::vector<std::string>& candidate_variants)
{
	if (county_variants.empty() || candidate_variants.empty())
	{
		return 0.0;
	}

	double best_signal = 0.0;
	for (const auto& county_variant: county_variants)
	{
		if (county_variant.size() < 4)
		{
			continue;
		}
		for (const auto& candidate_variant: candidate_variants)
		{
			if (candidate_variant.size() < 4)
			{
				continue;
			}
			if (county_variant == candidate_variant)
			{
				return 1.0;
			}

			if ((county_variant.find(candidate_variant) != std::string::npos ||
					 candidate_variant.find(county_variant) != std::string::npos) &&
				 std::min(county_variant.size(), candidate_variant.size()) >= 4)
			{
				best_signal = std::max(best_signal, 0.96);
			}

			const auto longest_size = std::max(county_variant.size(), candidate_variant.size());
			if (longest_size >= 6)
			{
				if (const auto edit_distance = boundedLevenshteinDistance(county_variant, candidate_variant, 1); edit_distance <= 1)
				{
					best_signal = std::max(best_signal, 0.94);
					continue;
				}
			}
			if (longest_size >= 7)
			{
				if (const auto edit_distance = boundedLevenshteinDistance(county_variant, candidate_variant, 2); edit_distance <= 2)
				{
					best_signal = std::max(best_signal, 0.90);
					continue;
				}
			}
			if (longest_size >= 8)
			{
				if (const auto edit_distance = boundedLevenshteinDistance(county_variant, candidate_variant, 3); edit_distance <= 3)
				{
					best_signal = std::max(best_signal, 0.85);
				}
			}
		}
	}

	return best_signal;
}

double candidateStrongNameSignal(const CountyCorrespondence& county, const CorrespondenceCandidate& candidate)
{
	return calculateStrongNameSignal(buildCountyNameVariants(county), buildCandidateNameVariants(candidate));
}

double bestCandidateStrongNameSignal(const CountyCorrespondence& county)
{
	double best_signal = 0.0;
	for (const auto& candidate: county.candidates)
	{
		best_signal = std::max(best_signal, candidateStrongNameSignal(county, candidate));
	}
	return best_signal;
}

void mergeFallbackCandidates(CountyCorrespondence& county,
	 std::vector<CorrespondenceCandidate> fallback_candidates,
	 const size_t maximum_candidates)
{
	if (fallback_candidates.empty())
	{
		return;
	}

	std::set<std::string> existing_definitions;
	for (const auto& candidate: county.candidates)
	{
		existing_definitions.insert(candidate.eu5_province_definition);
	}

	for (auto& candidate: fallback_candidates)
	{
		if (!existing_definitions.insert(candidate.eu5_province_definition).second)
		{
			continue;
		}
		county.candidates.push_back(std::move(candidate));
		if (county.candidates.size() >= maximum_candidates)
		{
			break;
		}
	}
}

std::optional<std::string> chooseAugmentedLocations(const CountyCorrespondence& county,
	 const double minimum_vote_share,
	 const double maximum_centroid_distance)
{
	if (county.candidates.empty())
	{
		return std::nullopt;
	}

	const auto& primary_candidate = county.candidates.front();
	if (primary_candidate.vote_share >= minimum_vote_share &&
		 primary_candidate.centroid_distance <= maximum_centroid_distance)
	{
		return common::join(primary_candidate.eu5_locations, "|");
	}

	struct ScoredCandidate
	{
		const CorrespondenceCandidate* candidate = nullptr;
		double affinity = 0.0;
		double composite = 0.0;
	};

	std::optional<ScoredCandidate> best_strong_name_candidate;
	for (size_t index = 0; index < county.candidates.size(); ++index)
	{
		const auto& candidate = county.candidates[index];
		const auto strong_signal = candidateStrongNameSignal(county, candidate);
		if (strong_signal < 0.90)
		{
			continue;
		}

		auto strong_name_distance_limit = std::max(maximum_centroid_distance, 0.04);
		if (strong_signal >= 0.96)
		{
			strong_name_distance_limit = std::max(strong_name_distance_limit, 0.25);
		}
		else if (strong_signal >= 0.94)
		{
			strong_name_distance_limit = std::max(strong_name_distance_limit, 0.12);
		}
		if (candidate.centroid_distance > strong_name_distance_limit)
		{
			continue;
		}

		const auto composite =
				strong_signal + candidate.vote_share * 0.20 - candidate.centroid_distance * 0.50 - static_cast<double>(index) * 0.01;
		if (!best_strong_name_candidate.has_value() || composite > best_strong_name_candidate->composite)
		{
			best_strong_name_candidate = {.candidate = &candidate, .affinity = strong_signal, .composite = composite};
		}
	}
	if (best_strong_name_candidate.has_value())
	{
		const auto& candidate = *best_strong_name_candidate->candidate;
		if ((best_strong_name_candidate->affinity >= 0.96 && candidate.vote_share >= 0.02) ||
				(best_strong_name_candidate->affinity >= 0.94 && candidate.vote_share >= 0.05) ||
				(best_strong_name_candidate->affinity >= 0.90 && candidate.vote_share >= 0.08))
		{
			return common::join(candidate.eu5_locations, "|");
		}
	}

	std::optional<ScoredCandidate> best_candidate;
	std::optional<ScoredCandidate> best_affinity_candidate;
	for (size_t index = 0; index < county.candidates.size(); ++index)
	{
		const auto& candidate = county.candidates[index];
		if (candidate.centroid_distance > maximum_centroid_distance)
		{
			continue;
		}

		const auto affinity = candidateNameAffinity(county, candidate);
		const auto composite = candidate.vote_share + affinity * 0.25 - static_cast<double>(index) * 0.01;
		if (!best_candidate.has_value() || composite > best_candidate->composite)
		{
			best_candidate = {.candidate = &candidate, .affinity = affinity, .composite = composite};
		}
		if (!best_affinity_candidate.has_value() || affinity > best_affinity_candidate->affinity ||
				(affinity == best_affinity_candidate->affinity &&
				 candidate.vote_share > best_affinity_candidate->candidate->vote_share))
		{
			best_affinity_candidate = {.candidate = &candidate, .affinity = affinity, .composite = composite};
		}
	}

	if (!best_candidate.has_value())
	{
		return std::nullopt;
	}

	if (best_affinity_candidate.has_value())
	{
		const auto& affinity_candidate = *best_affinity_candidate->candidate;
		if (best_affinity_candidate->affinity >= 0.85 && affinity_candidate.vote_share >= 0.15)
		{
			return common::join(affinity_candidate.eu5_locations, "|");
		}
		if (best_affinity_candidate->affinity >= 0.75 && affinity_candidate.vote_share >= 0.25 &&
				(best_candidate->affinity < 0.60 ||
				 affinity_candidate.vote_share + 0.08 >= primary_candidate.vote_share))
		{
			return common::join(affinity_candidate.eu5_locations, "|");
		}
	}

	const auto& candidate = *best_candidate->candidate;
	if (candidate.vote_share >= 0.48 || (best_candidate->affinity >= 0.85 && candidate.vote_share >= 0.20) ||
			(best_candidate->affinity >= 0.75 && candidate.vote_share >= 0.40))
	{
		return common::join(candidate.eu5_locations, "|");
	}

	return std::nullopt;
}

std::vector<ControlPoint> selectNearestControls(const std::vector<ControlPoint>& controls,
	 const double source_x,
	 const double source_y,
	 const size_t maximum_controls)
{
	struct DistanceControl
	{
		double distance_squared = 0.0;
		ControlPoint control;
	};

	std::vector<DistanceControl> nearest_controls;
	nearest_controls.reserve(controls.size());
	for (const auto& control: controls)
	{
		const auto dx = control.source_x - source_x;
		const auto dy = control.source_y - source_y;
		nearest_controls.push_back({.distance_squared = dx * dx + dy * dy, .control = control});
	}

	std::sort(nearest_controls.begin(), nearest_controls.end(), [](const auto& lhs, const auto& rhs) {
		if (lhs.distance_squared != rhs.distance_squared)
		{
			return lhs.distance_squared < rhs.distance_squared;
		}
		if (lhs.control.source_x != rhs.control.source_x)
		{
			return lhs.control.source_x < rhs.control.source_x;
		}
		return lhs.control.source_y < rhs.control.source_y;
	});

	if (nearest_controls.size() > maximum_controls)
	{
		nearest_controls.resize(maximum_controls);
	}

	std::vector<ControlPoint> selected_controls;
	selected_controls.reserve(nearest_controls.size());
	for (const auto& entry: nearest_controls)
	{
		selected_controls.push_back(entry.control);
	}
	return selected_controls;
}

std::vector<CorrespondenceCandidate> buildFallbackCandidates(const CountyCorrespondence& county,
	 const double mapped_centroid_x,
	 const double mapped_centroid_y,
	 const std::vector<Eu5ProvinceGroup>& eu5_groups,
	 const unsigned eu5_image_width,
	 const unsigned eu5_image_height)
{
	struct ScoredGroup
	{
		CorrespondenceCandidate candidate;
		double score = 0.0;
	};

	const auto county_variants = buildCountyNameVariants(county);
	std::vector<ScoredGroup> scored_groups;
	for (const auto& group: eu5_groups)
	{
		if (group.likely_water || group.stats.pixel_count == 0)
		{
			continue;
		}

		CorrespondenceCandidate candidate;
		candidate.eu5_province_definition = group.province_definition;
		candidate.eu5_locations = group.locations;
		candidate.eu5_display_names = group.display_names;
		candidate.centroid_distance = distance(mapped_centroid_x,
				mapped_centroid_y,
				group.stats.centroidX(eu5_image_width),
				group.stats.centroidY(eu5_image_height));

		const auto affinity = calculateNameAffinity(county_variants, buildCandidateNameVariants(candidate));
		if (affinity < 0.60)
		{
			continue;
		}

		const auto proximity = std::clamp(1.0 - candidate.centroid_distance / 0.05, 0.0, 1.0);
		const auto score = affinity * 0.75 + proximity * 0.25;
		if (score <= 0.0)
		{
			continue;
		}

		scored_groups.push_back({.candidate = std::move(candidate), .score = score});
	}

	std::sort(scored_groups.begin(), scored_groups.end(), [](const auto& lhs, const auto& rhs) {
		if (lhs.score != rhs.score)
		{
			return lhs.score > rhs.score;
		}
		if (lhs.candidate.centroid_distance != rhs.candidate.centroid_distance)
		{
			return lhs.candidate.centroid_distance < rhs.candidate.centroid_distance;
		}
		return lhs.candidate.eu5_province_definition < rhs.candidate.eu5_province_definition;
	});

	if (scored_groups.size() > 5)
	{
		scored_groups.resize(5);
	}

	double total_score = 0.0;
	for (const auto& scored_group: scored_groups)
	{
		total_score += scored_group.score;
	}

	std::vector<CorrespondenceCandidate> candidates;
	candidates.reserve(scored_groups.size());
	for (const auto& scored_group: scored_groups)
	{
		auto candidate = scored_group.candidate;
		candidate.votes = static_cast<uint64_t>(std::llround(scored_group.score * 1000.0));
		candidate.vote_share = total_score <= 0.0 ? 0.0 : scored_group.score / total_score;
		candidates.push_back(std::move(candidate));
	}

	return candidates;
}

}  // namespace

CorrespondenceResult MapCorrespondenceBuilder::build(const fs::path& ck3_game_path,
	 const fs::path& eu5_game_path,
	 const fs::path& control_province_mappings_path,
	 const fs::path& location_framework_path,
	 diagnostics::DiagnosticsReport& diagnostics) const
{
	const auto installed_titles = ck3::InstalledTitlesLoader().load(ck3_game_path);
	const auto ck3_definitions = loadCk3ProvinceDefinitions(ck3_game_path / "game/map_data/definition.csv");
	const auto eu5_framework_groups = loadEu5ProvinceGroupsFromFramework(location_framework_path);
	const auto eu5_named_location_colors =
			loadEu5NamedLocationColors(eu5_game_path / "in_game/map_data/named_locations");
	const auto control_mappings = loadControlMappings(control_province_mappings_path);
	const auto control_mapping_metadata = loadControlMappingMetadata(control_province_mappings_path);

	std::map<int, size_t> ck3_definition_index_by_id;
	std::unordered_map<uint32_t, size_t> ck3_definition_index_by_color;
	std::map<std::string, int> ck3_province_id_by_barony;
	std::vector<Ck3ProvinceAccumulator> ck3_provinces;
	ck3_provinces.reserve(ck3_definitions.size());
	for (const auto& definition: ck3_definitions)
	{
		ck3_definition_index_by_id[definition.id] = ck3_provinces.size();
		ck3_definition_index_by_color[definition.color] = ck3_provinces.size();
		ck3_province_id_by_barony[definition.barony_key] = definition.id;
		ck3_provinces.push_back({.definition = definition, .stats = {}, .eu5_votes = {}});
	}

	std::map<std::string, int> eu5_group_id_by_province_definition;
	std::vector<Eu5ProvinceGroup> eu5_groups;
	std::map<std::string, int> eu5_location_to_group_id;
	for (const auto& [province_definition, framework_group]: eu5_framework_groups)
	{
		const auto group_id = static_cast<int>(eu5_groups.size());
		eu5_group_id_by_province_definition[province_definition] = group_id;
		for (const auto& location_key: framework_group.locations)
		{
			eu5_location_to_group_id[location_key] = group_id;
		}
		eu5_groups.push_back({.province_definition = province_definition,
			 .locations = framework_group.locations,
			 .display_names = framework_group.display_names,
			 .stats = {},
			 .likely_water = isLikelyWaterProvince(province_definition)});
	}

	std::unordered_map<uint32_t, int> eu5_group_id_by_color;
	for (const auto& [color, location_key]: eu5_named_location_colors)
	{
		const auto it = eu5_location_to_group_id.find(location_key);
		if (it != eu5_location_to_group_id.end())
		{
			eu5_group_id_by_color[color] = it->second;
		}
	}

	const auto ck3_image = loadRgbPng(ck3_game_path / "game/map_data/provinces.png");
	const auto eu5_image = loadRgbPng(eu5_game_path / "in_game/map_data/locations.png");

	for (unsigned y = 0; y < eu5_image.height; ++y)
	{
		for (unsigned x = 0; x < eu5_image.width; ++x)
		{
			const auto color = eu5_image.colorAt(x, y);
			const auto group_it = eu5_group_id_by_color.find(color);
			if (group_it == eu5_group_id_by_color.end())
			{
				continue;
			}
			eu5_groups[group_it->second].stats.addPixel(x, y);
		}
	}

	for (unsigned y = 0; y < ck3_image.height; ++y)
	{
		for (unsigned x = 0; x < ck3_image.width; ++x)
		{
			const auto color = ck3_image.colorAt(x, y);
			const auto province_it = ck3_definition_index_by_color.find(color);
			if (province_it == ck3_definition_index_by_color.end())
			{
				continue;
			}
			ck3_provinces[province_it->second].stats.addPixel(x, y);
		}
	}

	CorrespondenceResult result;
	result.total_counties = installed_titles.counties.size();
	std::vector<int> province_to_county_index(ck3_provinces.size(), -1);
	for (const auto& [county_key, county]: installed_titles.counties)
	{
		CountyCorrespondence correspondence;
		correspondence.ck3_county = county_key;
		correspondence.display_name = county.display_name;

		RasterStats county_stats;
		VoteCounter county_votes;
		std::set<int> seen_province_ids;
		for (const auto& barony: county.baronies)
		{
			correspondence.barony_keys.push_back(barony.key);
			correspondence.barony_display_names.push_back(barony.display_name);
			int province_id = barony.province_id;
			if (province_id <= 0)
			{
				const auto province_id_it = ck3_province_id_by_barony.find(barony.key);
				if (province_id_it != ck3_province_id_by_barony.end())
				{
					province_id = province_id_it->second;
				}
			}
			if (province_id <= 0 || !seen_province_ids.insert(province_id).second)
			{
				continue;
			}

			correspondence.ck3_province_ids.push_back(province_id);
			const auto province_index_it = ck3_definition_index_by_id.find(province_id);
			if (province_index_it == ck3_definition_index_by_id.end())
			{
				continue;
			}
			const auto& province = ck3_provinces[province_index_it->second];
			county_stats.merge(province.stats);
			county_votes.merge(province.eu5_votes);
		}

		correspondence.raster_pixels = county_stats.pixel_count;
		correspondence.centroid_x = county_stats.centroidX(ck3_image.width);
		correspondence.centroid_y = county_stats.centroidY(ck3_image.height);
		if (county_stats.pixel_count > 0)
		{
			++result.counties_with_any_pixels;
		}
		for (const auto province_id: correspondence.ck3_province_ids)
		{
			const auto province_index_it = ck3_definition_index_by_id.find(province_id);
			if (province_index_it == ck3_definition_index_by_id.end())
			{
				continue;
			}
			province_to_county_index[province_index_it->second] = static_cast<int>(result.counties.size());
		}
		result.counties.push_back(std::move(correspondence));
	}

	std::vector<ControlPoint> controls;
	controls.reserve(control_mappings.size());
	for (const auto& county: result.counties)
	{
		if (county.raster_pixels == 0)
		{
			continue;
		}
		const auto control_it = control_mappings.find(county.ck3_county);
		if (control_it == control_mappings.end() || control_it->second.empty())
		{
			continue;
		}

		const auto metadata_it = control_mapping_metadata.find(county.ck3_county);
		if (metadata_it != control_mapping_metadata.end() && !metadata_it->second.allow_as_anchor)
		{
			continue;
		}

		const auto group_id = resolveControlGroupId(control_it->second, eu5_location_to_group_id, eu5_groups);
		if (!group_id.has_value())
		{
			continue;
		}
		const auto& group = eu5_groups[static_cast<size_t>(*group_id)];
		if (group.stats.pixel_count == 0)
		{
			continue;
		}
		controls.push_back({.source_x = county.centroid_x,
			 .source_y = county.centroid_y,
			 .target_x = group.stats.centroidX(eu5_image.width),
			 .target_y = group.stats.centroidY(eu5_image.height)});
	}

	const auto transform_fit = fitRobustAffineTransform(controls);
	const auto& transform = transform_fit.transform;
	diagnostics.info("MAP_CORRESPONDENCE_TRANSFORM",
		 "Fitted affine transform from " + std::to_string(transform_fit.input_controls) + " control counties; retained " +
				 std::to_string(transform_fit.inlier_controls) + " inliers with RMS residual " +
				 std::to_string(transform_fit.rms_error) + " and max residual " + std::to_string(transform_fit.max_error) +
				 ": x'=" + std::to_string(transform.ax) + "*x + " + std::to_string(transform.bx) + "*y + " +
				 std::to_string(transform.cx) + ", y'=" + std::to_string(transform.ay) + "*x + " + std::to_string(transform.by) +
				 "*y + " + std::to_string(transform.cy) + '.');

	std::vector<AffineTransform> county_transforms(result.counties.size(), transform);
	size_t counties_with_local_transform = 0;
	for (size_t county_index = 0; county_index < result.counties.size(); ++county_index)
	{
		const auto& county = result.counties[county_index];
		if (county.raster_pixels == 0)
		{
			continue;
		}

		const auto nearest_controls = selectNearestControls(controls, county.centroid_x, county.centroid_y, 64);
		if (nearest_controls.size() < 8)
		{
			continue;
		}

		county_transforms[county_index] = fitRobustAffineTransform(nearest_controls).transform;
		++counties_with_local_transform;
	}
	diagnostics.info("MAP_CORRESPONDENCE_LOCAL_TRANSFORMS",
		 "Built county-local affine transforms for " + std::to_string(counties_with_local_transform) +
				 " counties using up to 64 nearby control anchors each.");

	for (auto& province: ck3_provinces)
	{
		province.eu5_votes.counts.clear();
	}

	for (unsigned y = 0; y < ck3_image.height; ++y)
	{
		const auto normalized_y = static_cast<double>(y) / static_cast<double>(ck3_image.height);
		for (unsigned x = 0; x < ck3_image.width; ++x)
		{
			const auto color = ck3_image.colorAt(x, y);
			const auto province_it = ck3_definition_index_by_color.find(color);
			if (province_it == ck3_definition_index_by_color.end())
			{
				continue;
			}

			const auto county_index = province_to_county_index[province_it->second];
			const auto& county_transform =
					county_index >= 0 ? county_transforms[static_cast<size_t>(county_index)] : transform;

			const auto normalized_x = static_cast<double>(x) / static_cast<double>(ck3_image.width);
			auto [mapped_x, mapped_y] = county_transform.apply(normalized_x, normalized_y);
			mapped_x -= std::floor(mapped_x);
			mapped_y = std::clamp(mapped_y, 0.0, std::nextafter(1.0, 0.0));

			const auto sample_x =
					std::min(static_cast<unsigned>(mapped_x * static_cast<double>(eu5_image.width)), eu5_image.width - 1);
			const auto sample_y =
					std::min(static_cast<unsigned>(mapped_y * static_cast<double>(eu5_image.height)), eu5_image.height - 1);
			const auto eu5_color = eu5_image.colorAt(sample_x, sample_y);
			const auto group_it = eu5_group_id_by_color.find(eu5_color);
			if (group_it != eu5_group_id_by_color.end() &&
				 !eu5_groups[static_cast<size_t>(group_it->second)].likely_water)
			{
				ck3_provinces[province_it->second].eu5_votes.add(group_it->second);
			}
		}
	}

	size_t counties_with_fallback_candidates = 0;
	for (size_t county_index = 0; county_index < result.counties.size(); ++county_index)
	{
		auto& county = result.counties[county_index];
		VoteCounter county_votes;
		for (const auto province_id: county.ck3_province_ids)
		{
			const auto province_index_it = ck3_definition_index_by_id.find(province_id);
			if (province_index_it == ck3_definition_index_by_id.end())
			{
				continue;
			}
			county_votes.merge(ck3_provinces[province_index_it->second].eu5_votes);
		}

		auto ranked_votes = county_votes.counts;
		std::sort(ranked_votes.begin(), ranked_votes.end(), [](const auto& lhs, const auto& rhs) {
			if (lhs.second != rhs.second)
			{
				return lhs.second > rhs.second;
			}
			return lhs.first < rhs.first;
		});

		const auto total_votes = county_votes.total();
		auto [mapped_centroid_x, mapped_centroid_y] = county_transforms[county_index].apply(county.centroid_x, county.centroid_y);
		mapped_centroid_x -= std::floor(mapped_centroid_x);
		mapped_centroid_y = std::clamp(mapped_centroid_y, 0.0, std::nextafter(1.0, 0.0));
		for (const auto& [group_id, votes]: ranked_votes)
		{
			if (group_id < 0 || static_cast<size_t>(group_id) >= eu5_groups.size())
			{
				continue;
			}
			const auto& group = eu5_groups[static_cast<size_t>(group_id)];
			if (group.stats.pixel_count == 0)
			{
				continue;
			}

			CorrespondenceCandidate candidate;
			candidate.eu5_province_definition = group.province_definition;
			candidate.eu5_locations = group.locations;
			candidate.eu5_display_names = group.display_names;
			candidate.votes = votes;
			candidate.vote_share = total_votes == 0 ? 0.0 : static_cast<double>(votes) / static_cast<double>(total_votes);
			candidate.centroid_distance = distance(mapped_centroid_x,
					mapped_centroid_y,
					group.stats.centroidX(eu5_image.width),
					group.stats.centroidY(eu5_image.height));
			county.candidates.push_back(std::move(candidate));
			if (county.candidates.size() >= 5)
			{
				break;
			}
		}

		if (county.candidates.empty() && county.raster_pixels > 0)
		{
			county.candidates =
					buildFallbackCandidates(county, mapped_centroid_x, mapped_centroid_y, eu5_groups, eu5_image.width, eu5_image.height);
			if (!county.candidates.empty())
			{
				++counties_with_fallback_candidates;
			}
		}
		else if (county.raster_pixels > 0 && bestCandidateStrongNameSignal(county) < 0.85)
		{
			auto fallback_candidates =
					buildFallbackCandidates(county, mapped_centroid_x, mapped_centroid_y, eu5_groups, eu5_image.width, eu5_image.height);
			const auto original_candidate_count = county.candidates.size();
			mergeFallbackCandidates(county, std::move(fallback_candidates), 8);
			if (county.candidates.size() > original_candidate_count)
			{
				++counties_with_fallback_candidates;
			}
		}

		if (!county.candidates.empty())
		{
			++result.counties_with_candidates;
		}
	}

	diagnostics.info("MAP_CORRESPONDENCE_COUNTS",
		 "Built raster correspondence candidates for " + std::to_string(result.total_counties) + " CK3 counties. Counties with province pixels: " +
				 std::to_string(result.counties_with_any_pixels) + ", counties with candidates: " +
				 std::to_string(result.counties_with_candidates) + '.');
	diagnostics.info("MAP_CORRESPONDENCE_FALLBACKS",
		 "Generated or supplemented name-and-centroid fallback candidates for " +
				 std::to_string(counties_with_fallback_candidates) +
				 " counties whose direct raster vote set was empty or lacked a strong name signal.");
	return result;
}

void MapCorrespondenceBuilder::writeCandidatesCsv(const CorrespondenceResult& result, const fs::path& path) const
{
	std::ostringstream output;
	output << "ck3_county,display_name,raster_pixels,centroid_x,centroid_y,ck3_province_ids,barony_keys,candidate_rank,eu5_province_definition,eu5_locations,votes,vote_share,centroid_distance\n";
	for (const auto& county: result.counties)
	{
		if (county.candidates.empty())
		{
			output << csvEscape(county.ck3_county) << ',';
			output << csvEscape(county.display_name) << ',';
			output << county.raster_pixels << ',';
			output << county.centroid_x << ',';
			output << county.centroid_y << ',';
			output << csvEscape(joinInts(county.ck3_province_ids)) << ',';
			output << csvEscape(common::join(county.barony_keys, "|")) << ",\"\",\"\",\"\",,\n";
			continue;
		}

		for (size_t index = 0; index < county.candidates.size(); ++index)
		{
			const auto& candidate = county.candidates[index];
			output << csvEscape(county.ck3_county) << ',';
			output << csvEscape(county.display_name) << ',';
			output << county.raster_pixels << ',';
			output << county.centroid_x << ',';
			output << county.centroid_y << ',';
			output << csvEscape(joinInts(county.ck3_province_ids)) << ',';
			output << csvEscape(common::join(county.barony_keys, "|")) << ',';
			output << (index + 1) << ',';
			output << csvEscape(candidate.eu5_province_definition) << ',';
			output << csvEscape(common::join(candidate.eu5_locations, "|")) << ',';
			output << candidate.votes << ',';
			output << candidate.vote_share << ',';
			output << candidate.centroid_distance << '\n';
		}
	}
	common::writeTextFile(path, output.str(), common::TextEncoding::Utf8NoBom);
}

void MapCorrespondenceBuilder::writeTopMappingsCsv(const CorrespondenceResult& result,
	 const fs::path& path,
	 const double minimum_vote_share,
	 const double maximum_centroid_distance) const
{
	std::ostringstream output;
	output << "ck3_county,eu5_locations\n";
	for (const auto& county: result.counties)
	{
		if (county.candidates.empty())
		{
			continue;
		}
		const auto& candidate = county.candidates.front();
		if (candidate.vote_share < minimum_vote_share || candidate.centroid_distance > maximum_centroid_distance)
		{
			continue;
		}
		output << csvEscape(county.ck3_county) << ',' << csvEscape(common::join(candidate.eu5_locations, "|")) << '\n';
	}
	common::writeTextFile(path, output.str(), common::TextEncoding::Utf8NoBom);
}

void MapCorrespondenceBuilder::writeAugmentedMappingsCsv(const fs::path& base_mappings_path,
	 const CorrespondenceResult& result,
	 const fs::path& path,
	 const double minimum_vote_share,
	 const double maximum_centroid_distance) const
{
	std::map<std::string, std::string> merged_mappings;
	for (const auto& row: common::CsvReader::readFile(base_mappings_path))
	{
		const auto county_key = row.get("ck3_county");
		const auto eu5_locations = row.get("eu5_locations");
		if (county_key.empty() || eu5_locations.empty())
		{
			continue;
		}
		merged_mappings[county_key] = eu5_locations;
	}

	const auto manual_seed_path = path.parent_path() / "manual_seed_mappings.csv";
	if (fs::exists(manual_seed_path))
	{
		for (const auto& row: common::CsvReader::readFile(manual_seed_path))
		{
			const auto county_key = row.get("ck3_county");
			const auto eu5_locations = row.get("eu5_locations");
			if (county_key.empty() || eu5_locations.empty())
			{
				continue;
			}
			merged_mappings[county_key] = eu5_locations;
		}
	}

	for (const auto& county: result.counties)
	{
		if (merged_mappings.contains(county.ck3_county))
		{
			continue;
		}

		const auto selected_locations = chooseAugmentedLocations(county, minimum_vote_share, maximum_centroid_distance);
		if (!selected_locations.has_value())
		{
			continue;
		}
		merged_mappings[county.ck3_county] = *selected_locations;
	}

	std::ostringstream output;
	output << "ck3_county,eu5_locations\n";
	for (const auto& [county_key, eu5_locations]: merged_mappings)
	{
		output << csvEscape(county_key) << ',' << csvEscape(eu5_locations) << '\n';
	}
	common::writeTextFile(path, output.str(), common::TextEncoding::Utf8NoBom);
}

}  // namespace ck3eu5::maps
