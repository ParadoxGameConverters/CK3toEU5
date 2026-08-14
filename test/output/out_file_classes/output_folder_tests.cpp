#include <external/commonItems/external/googletest/googletest/include/gtest/gtest.h>
#include <gmock/gmock.h>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include "src/output/out_file_classes/output_file.hpp"
#include "src/output/out_file_classes/output_folder.hpp"
#include "src/output/utils/folder_manager.hpp"


namespace out
{

namespace

{
class MockFolderManager: public FolderManager
{
  public:
   MOCK_METHOD(void,  // NOLINT : clang-tidy doens't like gtest
       CreateFolder,
       (const std::filesystem::path& /*unused*/),
       (override));
};

class MockOutputFile: public OutputFileOrResource
{
  public:
   explicit MockOutputFile(std::string name): OutputFileOrResource(std::move(name)) {};

   MOCK_METHOD(void,  // NOLINT : clang-tidy doens't like gtest
       Create,
       (const std::filesystem::path& /*path*/),
       (override));
};

TEST(OutputFolderTest, RegisteredSingleFileCreateCalled)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string folder_name = "test_folder";
   const std::filesystem::path test_path = "test_path";

   MockFolderManager mock_folder_manager;
   OutputFolder output_folder(folder_name, mock_folder_manager);

   const std::string file_name = "test";
   auto single_file = std::make_unique<MockOutputFile>(file_name);

   // Must call all expects before the unique_ptr are moved
   EXPECT_CALL(*single_file, Create(test_path / folder_name));

   output_folder.RegisterFileOrResource(std::move(single_file));

   output_folder.CreateRecursive(test_path);
}

TEST(OutputFolderTest, RegisteredMultipleFilesCreateCalled)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string folder_name = "test_folder";
   const std::filesystem::path test_path = "test_path";

   MockFolderManager mock_folder_manager;
   OutputFolder output_folder(folder_name, mock_folder_manager);

   const std::string file_name = "test";
   auto single_file_0 = std::make_unique<MockOutputFile>(file_name);
   auto single_file_1 = std::make_unique<MockOutputFile>(file_name);
   auto single_file_2 = std::make_unique<MockOutputFile>(file_name);
   auto single_file_3 = std::make_unique<MockOutputFile>(file_name);

   // Must call all expects before the unique_ptr are moved
   EXPECT_CALL(*single_file_0, Create(test_path / folder_name));
   EXPECT_CALL(*single_file_1, Create(test_path / folder_name));
   EXPECT_CALL(*single_file_2, Create(test_path / folder_name));
   EXPECT_CALL(*single_file_3, Create(test_path / folder_name));

   output_folder.RegisterFileOrResource(std::move(single_file_0));
   output_folder.RegisterFileOrResource(std::move(single_file_1));
   output_folder.RegisterFileOrResource(std::move(single_file_2));
   output_folder.RegisterFileOrResource(std::move(single_file_3));

   output_folder.CreateRecursive(test_path);
}

TEST(OutputFolderTest, RegisteredSingleFileInSubfolderCreateCalled)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string folder_name = "test_folder";
   const std::string subfolder_name = "test_subfolder";
   const std::filesystem::path test_path = "test_path";

   MockFolderManager mock_folder_manager;
   OutputFolder output_folder(folder_name, mock_folder_manager);
   auto subfolder = std::make_unique<OutputFolder>(subfolder_name, mock_folder_manager);

   const std::string file_name = "test";
   auto single_file = std::make_unique<MockOutputFile>(file_name);

   // Must call all expects before the unique_ptr are moved
   EXPECT_CALL(*single_file, Create(test_path / folder_name / subfolder_name));

   subfolder->RegisterFileOrResource(std::move(single_file));
   output_folder.RegisterSubfolder(std::move(subfolder));

   output_folder.CreateRecursive(test_path);
}

TEST(OutputFolderTest, FolderCreated)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string folder_name = "test_folder";
   const std::filesystem::path test_path = "test_path";

   MockFolderManager mock_folder_manager;
   OutputFolder output_folder(folder_name, mock_folder_manager);

   EXPECT_CALL(mock_folder_manager, CreateFolder(test_path / folder_name));

   output_folder.CreateRecursive(test_path);
}

TEST(OutputFolderTest, SubfolderCreated)  // NOLINT : clang-tidy doens't like gtest
{
   const std::string folder_name = "test_folder";
   const std::string subfolder_name = "test_subfolder";
   const std::filesystem::path test_path = "test_path";

   MockFolderManager mock_folder_manager;
   OutputFolder output_folder(folder_name, mock_folder_manager);
   auto subfolder = std::make_unique<OutputFolder>(subfolder_name, mock_folder_manager);

   output_folder.RegisterSubfolder(std::move(subfolder));

   EXPECT_CALL(mock_folder_manager, CreateFolder(test_path / folder_name));
   EXPECT_CALL(mock_folder_manager, CreateFolder(test_path / folder_name / subfolder_name));

   output_folder.CreateRecursive(test_path);
}



}  // namespace

}  // namespace out