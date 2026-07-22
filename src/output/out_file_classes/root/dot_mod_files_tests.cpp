#include <external/commonItems/GameVersion.h>
#include <external/commonItems/external/googletest/googletest/include/gtest/gtest.h>
#include <gmock/gmock.h>

#include <filesystem>
#include <string>

#include "dot_mod_files.hpp"
#include "src/output/utils/file_writer.hpp"



namespace out
{

namespace
{

class MockFileWriter: public FileWriter
{
  public:
   MOCK_METHOD(void,  // NOLINT : clang-tidy doens't like gtest
       CreateEmptyAndWrite,
       (const std::filesystem::path&, const std::string&),
       (override));
};

TEST(DotModFiles, CorrectFileConentWritten)  // NOLINT : clang-tidy doens't like gtest
{
   MockFileWriter mock;

   const std::string name = "zaba";
   const std::string expected_mod_file_content =
       "name = \"Converted - zaba\"\n"
       "path = \"mod/zaba/\"\n"
       "user_dir = \"zaba_user_dir\"\n"
       "replace_path=\"common/countries\"\n"
       "replace_path=\"common/national_focus\"\n"
       "replace_path=\"common/peace_conference/ai_peace\"\n"
       "replace_path=\"common/peace_conference/cost_modifiers\"\n"
       "replace_path=\"events\"\n"
       "replace_path=\"history/countries\"\n"
       "replace_path=\"history/states\"\n"
       "replace_path=\"history/units\"\n"
       "replace_path=\"map/supplyareas\"\n"
       "replace_path=\"map/strategicregions\"\n"
       "supported_version=\"*\"";

   const std::string expected_descriptor_file_content =
       "name = \"Converted - zaba\"\n"
       "replace_path=\"common/countries\"\n"
       "replace_path=\"common/national_focus\"\n"
       "replace_path=\"common/peace_conference/ai_peace\"\n"
       "replace_path=\"common/peace_conference/cost_modifiers\"\n"
       "replace_path=\"events\"\n"
       "replace_path=\"history/countries\"\n"
       "replace_path=\"history/states\"\n"
       "replace_path=\"history/units\"\n"
       "replace_path=\"map/supplyareas\"\n"
       "replace_path=\"map/strategicregions\"\n"
       "supported_version=\"*\"";
   const std::filesystem::path expected_path = std::filesystem::path("folder") / "zaba.mod";
   const std::filesystem::path expected_descriptor_path = std::filesystem::path("folder") / "descriptor.mod";

   DotModFiles file_resource(name, &mock, GameVersion(""));

   EXPECT_CALL(mock, CreateEmptyAndWrite(expected_path, expected_mod_file_content));
   EXPECT_CALL(mock, CreateEmptyAndWrite(expected_descriptor_path, expected_descriptor_file_content));

   file_resource.Create("folder");
}

}  // namespace

}  // namespace out