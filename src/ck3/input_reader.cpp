#include "ck3/input_reader.h"

#include "ck3/raw_save_normalizer.h"
#include "common/filesystem_utils.h"
#include "common/process.h"
#include "common/string_utils.h"

namespace ck3eu5::ck3 {

std::string InputReader::read(const config::Configuration& configuration) const
{
	if (!configuration.preprocessor_command.empty())
	{
		const auto command =
				common::replaceAll(configuration.preprocessor_command, "{input}", configuration.ck3_input_path.string());
		return common::executeCommandCaptureStdout(command);
	}

	if (configuration.auto_normalize_raw_ck3 && configuration.ck3_input_path.extension() == ".ck3")
	{
		return normalizeSaveFile(configuration.ck3_input_path);
	}

	return common::readTextFile(configuration.ck3_input_path);
}

}  // namespace ck3eu5::ck3
