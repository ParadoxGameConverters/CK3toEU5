#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace ck3eu5::diagnostics {

enum class Severity
{
	Info,
	Warning,
	Error
};

struct Issue
{
	Severity severity;
	std::string code;
	std::string message;
};

class DiagnosticsReport
{
  public:
	void add(Severity severity, std::string code, std::string message);
	void info(std::string code, std::string message);
	void warning(std::string code, std::string message);
	void error(std::string code, std::string message);

	[[nodiscard]] bool hasErrors() const;
	[[nodiscard]] const std::vector<Issue>& issues() const;
	[[nodiscard]] std::string summary() const;
	void write(const std::filesystem::path& path) const;

  private:
	std::vector<Issue> issues_;
};

}  // namespace ck3eu5::diagnostics
