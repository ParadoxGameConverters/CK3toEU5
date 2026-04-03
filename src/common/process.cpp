#include "common/process.h"

#include <array>
#include <cstdio>
#include <stdexcept>

namespace ck3eu5::common {

std::string executeCommandCaptureStdout(const std::string& command)
{
	std::array<char, 256> buffer{};
	std::string output;

#if defined(_WIN32)
	FILE* pipe = _popen(command.c_str(), "r");
#else
	FILE* pipe = popen(command.c_str(), "r");
#endif
	if (!pipe)
	{
		throw std::runtime_error("Failed to launch command: " + command);
	}

	try
	{
		while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
		{
			output += buffer.data();
		}
	}
	catch (...)
	{
#if defined(_WIN32)
		_pclose(pipe);
#else
		pclose(pipe);
#endif
		throw;
	}

#if defined(_WIN32)
	const int exit_code = _pclose(pipe);
#else
	const int exit_code = pclose(pipe);
#endif
	if (exit_code != 0)
	{
		throw std::runtime_error("Command failed with exit code " + std::to_string(exit_code) + ": " + command);
	}
	return output;
}

}  // namespace ck3eu5::common
