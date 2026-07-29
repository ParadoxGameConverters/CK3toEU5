#include <external/commonItems/external/googletest/googletest/include/gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

#include "src/output/utils/file_writer_impl.hpp"


namespace out
{

namespace
{

class FileManagerTest: public ::testing::Test
{
  protected:
   void SetUp() override
   {
      std::filesystem::remove_all(temp_testing_dir_);
      std::filesystem::create_directory(temp_testing_dir_);
   }

   void TearDown() override { std::filesystem::remove_all(temp_testing_dir_); }

   std::string temp_testing_dir_ = "output_tests_tmp";  // NOLINT: misc-non-private-member-variables-in-classes
};

TEST_F(FileManagerTest, CorrectFileConentWritten)  // NOLINT : clang-tidy doens't like gtest
{
   FileWriterImpl file_writer;
   const std::string test_content = "test content";
   const std::string file_path = std::format("{}/test_file.txt", temp_testing_dir_);

   ASSERT_FALSE(std::filesystem::exists(file_path));

   file_writer.CreateEmptyAndWrite(file_path, test_content);

   std::ifstream mod_file(file_path);
   ASSERT_TRUE(mod_file.is_open());
   std::stringstream mod_file_stream;
   std::copy(std::istreambuf_iterator<char>(mod_file),
       std::istreambuf_iterator<char>(),
       std::ostreambuf_iterator<char>(mod_file_stream));
   mod_file.close();

   EXPECT_EQ(mod_file_stream.str(), test_content);
}

TEST_F(FileManagerTest, FailsWhenCreatingSameFileTwice)  // NOLINT : clang-tidy doens't like gtest
{
   FileWriterImpl file_writer;
   const std::string test_content = "test content";
   const std::string file_path = std::format("{}/test_file.txt", temp_testing_dir_);

   ASSERT_FALSE(std::filesystem::exists(file_path));

   file_writer.CreateEmptyAndWrite(file_path, test_content);

   ASSERT_TRUE(std::filesystem::exists(file_path));

   EXPECT_THROW(file_writer.CreateEmptyAndWrite(file_path, test_content),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}

TEST_F(FileManagerTest, FailsWhenWritingToMissingFolder)  // NOLINT : clang-tidy doens't like gtest
{
   FileWriterImpl file_writer;
   const std::string test_content = "test content";
   const std::string file_path = "nonexistant_folder/test_file.txt";

   EXPECT_THROW(file_writer.CreateEmptyAndWrite(file_path, test_content),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}

}  // namespace

}  // namespace out