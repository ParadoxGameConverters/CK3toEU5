#include <external/commonItems/external/googletest/googletest/include/gtest/gtest.h>
#include <gmock/gmock.h>

#include <filesystem>
#include <string>

#include "generic_advisors.hpp"
#include "src/output/utils/file_writer.hpp"


namespace out
{


namespace
{



class MockFileWriter: public FileWriter
{
  public:
   MOCK_METHOD(void,
       CreateEmptyAndWrite,
       (const std::filesystem::path&, const std::string&),
       (override));  // NOLINT : clang-tidy doens't like gtest
};

TEST(GenericAdvisorsTests, CorrectFileConentWritten)  // NOLINT : clang-tidy doens't like gtest
{
   MockFileWriter mock;

   const std::string name = "advisors";
   const std::string expected_mod_file_content = "zaba 123 321";
   const std::filesystem::path expected_path = "folder\\advisors.txt";

   AdvisorFile file_resource(name, &mock);

   EXPECT_CALL(mock, CreateEmptyAndWrite(expected_path, expected_mod_file_content));

   file_resource.Create("folder");
}

}  // namespace

}  // namespace out