#include "ck3/raw_save_normalizer.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
	try
	{
		if (argc < 2)
		{
			throw std::runtime_error("Usage: ck3_to_eu5_normalize <save.ck3>");
		}

		std::cout << ck3eu5::ck3::normalizeSaveFile(fs::path(argv[1]));
		return 0;
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << '\n';
		return 1;
	}
}
