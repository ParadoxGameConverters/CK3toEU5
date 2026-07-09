# CK3toEU5


## Building
This project uses CMake and can be build using any modern C++ toolchain that supports C++23, though Visual Studio is the most used and tested.

To start, first clone the repository. Then open a terminal window to the location where it was cloned and run the command `git submodule update --init --recursive`. When this has finished, open the folder in your IDE of choice.

There are three CMake configurations: x64 release, x64 debug, and x64 clang-tidy. The last runs a series of static analysis checks on the code base.

When desired changes have been made, open a pull request on github. A number of automatic checks will run: a windows build, a linux build, a clang-tidy scan, a clang-format check, a check that data files have properly paired curly-braces, a scan via Codacy, and a scan via CodeFactor. When those have passed and one of the code owners has approved the PR, it can be merged.
