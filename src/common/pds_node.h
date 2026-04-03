#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ck3eu5::common {

class PdsNode
{
  public:
	static PdsNode makeScalar(std::string value);
	static PdsNode makeBlock();

	[[nodiscard]] bool isScalar() const;
	[[nodiscard]] const std::string& scalar() const;
	[[nodiscard]] const std::vector<std::pair<std::string, PdsNode>>& properties() const;
	[[nodiscard]] const std::vector<PdsNode>& items() const;

	void addProperty(std::string key, PdsNode value);
	void addItem(PdsNode value);

	const PdsNode* get(std::string_view key) const;
	std::vector<const PdsNode*> getAll(std::string_view key) const;

	std::string scalarOr(std::string_view fallback = "") const;
	int asInt(int fallback = 0) const;
	double asDouble(double fallback = 0.0) const;
	bool asBool(bool fallback = false) const;

	std::string getString(std::string_view key, std::string_view fallback = "") const;
	int getInt(std::string_view key, int fallback = 0) const;
	double getDouble(std::string_view key, double fallback = 0.0) const;
	bool getBool(std::string_view key, bool fallback = false) const;
	std::vector<std::string> getListOfScalars(std::string_view key) const;

  private:
	bool scalar_ = false;
	std::string scalar_value_;
	std::vector<std::pair<std::string, PdsNode>> properties_;
	std::vector<PdsNode> items_;
};

}  // namespace ck3eu5::common
