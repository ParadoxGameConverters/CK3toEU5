#include "common/logger.h"

#include <iostream>

namespace ck3eu5::common {

bool Logger::verbose_ = false;

namespace {
std::string_view prefix(const LogLevel level)
{
	switch (level)
	{
		case LogLevel::Debug:
			return "[DEBUG]";
		case LogLevel::Info:
			return "[INFO ]";
		case LogLevel::Warning:
			return "[WARN ]";
		case LogLevel::Error:
			return "[ERROR]";
	}
	return "[INFO ]";
}
}  // namespace

void Logger::setVerbose(const bool enabled)
{
	verbose_ = enabled;
}

void Logger::log(const LogLevel level, std::string_view message)
{
	if (level == LogLevel::Debug && !verbose_)
	{
		return;
	}
	std::ostream& stream = level == LogLevel::Error ? std::cerr : std::cout;
	stream << prefix(level) << ' ' << message << '\n';
}

void Logger::debug(std::string_view message)
{
	log(LogLevel::Debug, message);
}

void Logger::info(std::string_view message)
{
	log(LogLevel::Info, message);
}

void Logger::warning(std::string_view message)
{
	log(LogLevel::Warning, message);
}

void Logger::error(std::string_view message)
{
	log(LogLevel::Error, message);
}

}  // namespace ck3eu5::common
