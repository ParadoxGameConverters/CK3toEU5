#include "diagnostics/diagnostics_report.h"

#include "common/filesystem_utils.h"

#include <sstream>

namespace ck3eu5::diagnostics {

namespace {
std::string_view label(const Severity severity)
{
	switch (severity)
	{
		case Severity::Info:
			return "INFO";
		case Severity::Warning:
			return "WARNING";
		case Severity::Error:
			return "ERROR";
	}
	return "INFO";
}
}  // namespace

void DiagnosticsReport::add(const Severity severity, std::string code, std::string message)
{
	issues_.push_back(Issue{severity, std::move(code), std::move(message)});
}

void DiagnosticsReport::info(std::string code, std::string message)
{
	add(Severity::Info, std::move(code), std::move(message));
}

void DiagnosticsReport::warning(std::string code, std::string message)
{
	add(Severity::Warning, std::move(code), std::move(message));
}

void DiagnosticsReport::error(std::string code, std::string message)
{
	add(Severity::Error, std::move(code), std::move(message));
}

bool DiagnosticsReport::hasErrors() const
{
	for (const auto& issue: issues_)
	{
		if (issue.severity == Severity::Error)
		{
			return true;
		}
	}
	return false;
}

const std::vector<Issue>& DiagnosticsReport::issues() const
{
	return issues_;
}

std::string DiagnosticsReport::summary() const
{
	std::ostringstream out;
	size_t info_count = 0;
	size_t warning_count = 0;
	size_t error_count = 0;

	for (const auto& issue: issues_)
	{
		switch (issue.severity)
		{
			case Severity::Info:
				++info_count;
				break;
			case Severity::Warning:
				++warning_count;
				break;
			case Severity::Error:
				++error_count;
				break;
		}
	}

	out << "CK3ToEU5 diagnostics summary\n";
	out << "Infos: " << info_count << '\n';
	out << "Warnings: " << warning_count << '\n';
	out << "Errors: " << error_count << "\n\n";

	for (const auto& issue: issues_)
	{
		out << '[' << label(issue.severity) << "] " << issue.code << ": " << issue.message << '\n';
	}
	return out.str();
}

void DiagnosticsReport::write(const std::filesystem::path& path) const
{
	common::writeTextFile(path, summary(), common::TextEncoding::Utf8NoBom);
}

}  // namespace ck3eu5::diagnostics
