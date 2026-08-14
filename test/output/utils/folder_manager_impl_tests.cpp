#include <external/commonItems/external/googletest/googletest/include/gtest/gtest.h>

#include <filesystem>
#include <format>
#include <stdexcept>
#include <string>

#include "src/output/utils/folder_manager_impl.hpp"


namespace out
{

namespace
{

class FolderManagerTest: public ::testing::Test
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

TEST_F(FolderManagerTest, FolderCreated)  // NOLINT : clang-tidy doens't like gtest
{
   FolderManagerImpl folder_manager;
   const std::string folder_path = std::format("{}/test_folder", temp_testing_dir_);

   ASSERT_FALSE(std::filesystem::exists(folder_path));

   folder_manager.CreateFolder(folder_path);

   ASSERT_TRUE(std::filesystem::exists(folder_path));
}

TEST_F(FolderManagerTest, FolderCreatedAndRemoved)  // NOLINT : clang-tidy doens't like gtest
{
   FolderManagerImpl folder_manager;
   const std::string folder_path = std::format("{}/test_folder", temp_testing_dir_);

   ASSERT_FALSE(std::filesystem::exists(folder_path));

   folder_manager.CreateFolder(folder_path);

   ASSERT_TRUE(std::filesystem::exists(folder_path));

   folder_manager.RemoveFolder(folder_path);

   ASSERT_FALSE(std::filesystem::exists(folder_path));
}

TEST_F(FolderManagerTest, DuplicateFolderCreationThrowsError)  // NOLINT : clang-tidy doens't like gtest
{
   FolderManagerImpl folder_manager;
   const std::string folder_path = std::format("{}/test_folder", temp_testing_dir_);

   ASSERT_FALSE(std::filesystem::exists(folder_path));

   folder_manager.CreateFolder(folder_path);

   ASSERT_TRUE(std::filesystem::exists(folder_path));

   EXPECT_THROW(folder_manager.CreateFolder(folder_path),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}

}  // namespace

}  // namespace out