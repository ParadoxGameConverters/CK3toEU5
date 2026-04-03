#include "convert/tag_generator.h"

#include "common/string_utils.h"

#include <cctype>

namespace ck3eu5::convert {

TagGenerator::TagGenerator(std::set<std::string> reserved_tags): used_tags_(std::move(reserved_tags)) {}

std::string TagGenerator::lettersOnlyUpper(const std::string& value)
{
	std::string output;
	for (const char c: value)
	{
		if (std::isalpha(static_cast<unsigned char>(c)))
		{
			output.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
		}
	}
	return output;
}

std::string TagGenerator::generateFromSource(const std::string& source_key) const
{
	std::string stripped = source_key;
	for (const std::string_view prefix: {"b_", "c_", "d_", "k_", "e_"})
	{
		if (stripped.rfind(prefix, 0) == 0)
		{
			stripped = stripped.substr(prefix.size());
			break;
		}
	}
	const auto segments = common::split(stripped, '_');
	std::string candidate;
	for (const auto& segment: segments)
	{
		if (!segment.empty())
		{
			candidate.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(segment.front()))));
		}
		if (candidate.size() == 3)
		{
			return candidate;
		}
	}
	const auto letters = lettersOnlyUpper(stripped);
	for (const char c: letters)
	{
		if (candidate.size() == 3)
		{
			break;
		}
		candidate.push_back(c);
	}
	while (candidate.size() < 3)
	{
		candidate.push_back('X');
	}
	return candidate.substr(0, 3);
}

std::string TagGenerator::nextSyntheticTag()
{
	while (true)
	{
		const char first = static_cast<char>('X');
		const char second = static_cast<char>('A' + ((synthetic_counter_ / 26) % 26));
		const char third = static_cast<char>('A' + (synthetic_counter_ % 26));
		++synthetic_counter_;
		std::string tag;
		tag.push_back(first);
		tag.push_back(second);
		tag.push_back(third);
		if (!used_tags_.contains(tag))
		{
			return tag;
		}
	}
}

std::string TagGenerator::getOrCreate(const std::string& source_key)
{
	auto candidate = generateFromSource(source_key);
	if (!used_tags_.contains(candidate))
	{
		used_tags_.insert(candidate);
		return candidate;
	}
	candidate = nextSyntheticTag();
	used_tags_.insert(candidate);
	return candidate;
}

}  // namespace ck3eu5::convert
