#include <external/commonItems/ConverterVersion.h>
#include <external/commonItems/external/googletest/googletest/include/gtest/gtest.h>
#include <gmock/gmock.h>

#include <filesystem>
#include <string>

#include "metadata.hpp"
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

   const std::string name = "zaba saving";
   const std::string expected_mod_file_content =
       "{\n"
       "\t\"name\": \"zaba saving\",\n"
       "\t\"id\": \"zabasaving\",\n"
       "\t\"version\": \"\",\n"
       "\t\"supported_game_version\": \"*\",\n"
       "\t\"short_description\": \"Mod converting CK3 to EU5\",\n"
       "\t\"tags\": [\"Alternative History\", \"Overhaul\"],\n"
       "\t\"relationships\": [],\n"
       "\t\"game_custom_data\": {\n"
       "\t\t\"replace_paths\": [\n"
       "\t\t]\n"
       "\t}\n"
       "}";

   const std::filesystem::path expected_path = std::filesystem::path("folder") / "metadata.json";

   MetadataFile file_resource(name, mock, commonItems::ConverterVersion());

   EXPECT_CALL(mock, CreateEmptyAndWrite(expected_path, expected_mod_file_content));

   file_resource.Create("folder");
}

}  // namespace

}  // namespace out