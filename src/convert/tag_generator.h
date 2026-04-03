#pragma once

#include <set>
#include <string>

namespace ck3eu5::convert {

class TagGenerator
{
  public:
	explicit TagGenerator(std::set<std::string> reserved_tags);

	std::string getOrCreate(const std::string& source_key);

  private:
	std::string generateFromSource(const std::string& source_key) const;
	std::string nextSyntheticTag();
	static std::string lettersOnlyUpper(const std::string& value);

	std::set<std::string> used_tags_;
	size_t synthetic_counter_ = 0;
};

}  // namespace ck3eu5::convert
