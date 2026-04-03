#include "common/pds_node.h"

#include "common/string_utils.h"

#include <stdexcept>

namespace ck3eu5::common {

PdsNode PdsNode::makeScalar(std::string value)
{
	PdsNode node;
	node.scalar_ = true;
	node.scalar_value_ = std::move(value);
	return node;
}

PdsNode PdsNode::makeBlock()
{
	return PdsNode{};
}

bool PdsNode::isScalar() const
{
	return scalar_;
}

const std::string& PdsNode::scalar() const
{
	if (!scalar_)
	{
		throw std::runtime_error("PdsNode is not scalar");
	}
	return scalar_value_;
}

const std::vector<std::pair<std::string, PdsNode>>& PdsNode::properties() const
{
	return properties_;
}

const std::vector<PdsNode>& PdsNode::items() const
{
	return items_;
}

void PdsNode::addProperty(std::string key, PdsNode value)
{
	properties_.emplace_back(std::move(key), std::move(value));
}

void PdsNode::addItem(PdsNode value)
{
	items_.push_back(std::move(value));
}

const PdsNode* PdsNode::get(std::string_view key) const
{
	for (const auto& [candidate, value]: properties_)
	{
		if (candidate == key)
		{
			return &value;
		}
	}
	return nullptr;
}

std::vector<const PdsNode*> PdsNode::getAll(std::string_view key) const
{
	std::vector<const PdsNode*> result;
	for (const auto& [candidate, value]: properties_)
	{
		if (candidate == key)
		{
			result.push_back(&value);
		}
	}
	return result;
}

std::string PdsNode::scalarOr(std::string_view fallback) const
{
	return scalar_ ? scalar_value_ : std::string(fallback);
}

int PdsNode::asInt(const int fallback) const
{
	if (!scalar_)
	{
		return fallback;
	}
	if (const auto parsed = parseInt(scalar_value_); parsed.has_value())
	{
		return *parsed;
	}
	return fallback;
}

double PdsNode::asDouble(const double fallback) const
{
	if (!scalar_)
	{
		return fallback;
	}
	if (const auto parsed = parseDouble(scalar_value_); parsed.has_value())
	{
		return *parsed;
	}
	return fallback;
}

bool PdsNode::asBool(const bool fallback) const
{
	if (!scalar_)
	{
		return fallback;
	}
	return parseBool(scalar_value_, fallback);
}

std::string PdsNode::getString(std::string_view key, std::string_view fallback) const
{
	if (const auto* child = get(key))
	{
		return child->scalarOr(fallback);
	}
	return std::string(fallback);
}

int PdsNode::getInt(std::string_view key, const int fallback) const
{
	if (const auto* child = get(key))
	{
		return child->asInt(fallback);
	}
	return fallback;
}

double PdsNode::getDouble(std::string_view key, const double fallback) const
{
	if (const auto* child = get(key))
	{
		return child->asDouble(fallback);
	}
	return fallback;
}

bool PdsNode::getBool(std::string_view key, const bool fallback) const
{
	if (const auto* child = get(key))
	{
		return child->asBool(fallback);
	}
	return fallback;
}

std::vector<std::string> PdsNode::getListOfScalars(std::string_view key) const
{
	std::vector<std::string> result;
	if (const auto* child = get(key))
	{
		if (child->isScalar())
		{
			result.push_back(child->scalar());
			return result;
		}
		for (const auto& item: child->items())
		{
			result.push_back(item.scalarOr());
		}
	}
	return result;
}

}  // namespace ck3eu5::common
