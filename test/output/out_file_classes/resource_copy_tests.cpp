#include <external/commonItems/external/googletest/googletest/include/gtest/gtest.h>

#include <filesystem>
#include <string>

#include "src/output/out_file_classes/resource_copy.hpp"


namespace out
{

namespace
{

class ResourceCopyTest: public ::testing::Test
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

TEST_F(ResourceCopyTest, FileCopied)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string name = "output_file_name.txt";
   const std::filesystem::path resource_path = "test_files/test_copy/some_file.txt";
   CopyResource copy_resource(name, resource_path);

   ASSERT_FALSE(std::filesystem::exists(std::filesystem::path(temp_testing_dir_) / name));

   copy_resource.Create(temp_testing_dir_);

   ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(temp_testing_dir_) / name));
}

TEST_F(ResourceCopyTest, FolderCopied)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string name = "test_folder";
   const std::filesystem::path resource_path = "test_files/test_copy";
   CopyResource copy_resource(name, resource_path);

   ASSERT_FALSE(std::filesystem::exists(std::filesystem::path(temp_testing_dir_) / name));

   copy_resource.Create(temp_testing_dir_);

   ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(temp_testing_dir_) / name));
   ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(temp_testing_dir_) / name / "some_file.txt"));
}

}  // namespace

}  // namespace out