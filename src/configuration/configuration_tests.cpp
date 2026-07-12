#include <external/commonItems/external/googletest/googlemock/include/gmock/gmock-matchers.h>
#include <external/commonItems/external/googletest/googletest/include/gtest/gtest.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "src/configuration/configuration.hpp"
#include "src/configuration/configuration_loader.hpp"

// Tests copied from Vic3toEU4 - these aren't really unit tests, more like e2e tests, but they should suffice

namespace configuration
{

TEST(ConfigurationTest, DefaultsAreDefaulted)  // NOLINT : clang-tidy doens't like gtest
{
   const auto configuration = LoadConfiguration("test_files/configuration/blank_configuration.txt");

   EXPECT_TRUE(configuration.GetCK3Directory().empty());
   EXPECT_TRUE(configuration.GetCK3DocDirectory().empty());
   EXPECT_TRUE(configuration.GetEU5Directory().empty());
   EXPECT_TRUE(configuration.GetEU5ModPath().empty());
   EXPECT_EQ(configuration.GetSaveGamePath(),
       "test_save.ck3");  // a missing save would throw an exception, so this is set in the 'blank' config
   EXPECT_FALSE(configuration.GetDebug());
   EXPECT_EQ(configuration.GetOutputName(), "test_save");  // if not specified, derived from the save
}


TEST(ConfigurationTest, ItemsCanBeImported)  // NOLINT : clang-tidy doens't like gtest
{
   const auto configuration = LoadConfiguration("test_files/configuration/test_configuration.txt");

   EXPECT_EQ(configuration.GetCK3Directory(), R"(test_files/test_folders/ck3_folder)");
   EXPECT_EQ(configuration.GetCK3DocDirectory(), "ck3_doc_directory");
   EXPECT_EQ(configuration.GetEU5Directory(), R"(test_files/test_folders/eu5_folder)");
   EXPECT_EQ(configuration.GetEU5ModPath(), "eu5_mod_directory");
   EXPECT_EQ(configuration.GetSaveGamePath(), "test_save.ck3");
   EXPECT_TRUE(configuration.GetDebug());
   EXPECT_EQ(configuration.GetOutputName(), "test_output_name");
}


TEST(ConfigurationTest, ItemsAreLoggedWhenImported)  // NOLINT : clang-tidy doens't like gtest
{
   const std::stringstream log;
   std::streambuf* cout_buffer = std::cout.rdbuf();
   std::cout.rdbuf(log.rdbuf());

   const auto ignore = LoadConfiguration("test_files/configuration/test_configuration.txt");

   EXPECT_THAT(log.str(),
       testing::HasSubstr(R"(Crusader Kings 3 install path is "test_files/test_folders/ck3_folder")"));
   EXPECT_THAT(log.str(), testing::HasSubstr(R"(Crusader Kings 3 documents directory is "ck3_doc_directory")"));
   EXPECT_THAT(log.str(),
       testing::HasSubstr(R"(Europa Universalis 5 install path is "test_files/test_folders/eu5_folder")"));
   EXPECT_THAT(log.str(), testing::HasSubstr(R"(Europa Universalis 5 mod path is "eu5_mod_directory")"));
   EXPECT_THAT(log.str(), testing::HasSubstr(R"(Save game is "test_save.ck3")"));
   EXPECT_THAT(log.str(), testing::HasSubstr(R"(Debug is active)"));
   EXPECT_THAT(log.str(), testing::HasSubstr(R"(Using output name test_output_name)"));

   std::cout.rdbuf(cout_buffer);
}


TEST(ConfigurationTest, OutputNameIsFromSave)  // NOLINT : clang-tidy doens't like gtest
{
   const auto configuration = LoadConfiguration("test_files/configuration/output_name_from_save_configuration.txt");

   EXPECT_EQ(configuration.GetOutputName(), "test_save_with_spaces");
}


TEST(ConfigurationTest, CustomOutputOverridesSaveOutputName)  // NOLINT : clang-tidy doens't like gtest
{
   const auto configuration = LoadConfiguration("test_files/configuration/output_name_override.txt");

   EXPECT_EQ(configuration.GetOutputName(), "path_has_not__been__removed__override_name_with_spaces");
}


TEST(ConfigurationTest, ExceptionForMissingCk3Directory)  // NOLINT : clang-tidy doens't like gtest
{
   const auto configuration = LoadConfiguration("test_files/configuration/missing_ck3_directory.txt");
   EXPECT_THROW(configuration.Validate(commonItems::ConverterVersion()),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}


TEST(ConfigurationTest, ExceptionForBadVic3Directory)  // NOLINT : clang-tidy doens't like gtest
{
   const auto configuration = LoadConfiguration("test_files/configuration/bad_ck3_directory.txt");
   EXPECT_THROW(configuration.Validate(commonItems::ConverterVersion()),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}


TEST(ConfigurationTest, ExceptionForMissingEU5Directory)  // NOLINT : clang-tidy doens't like gtest
{
   const auto configuration = LoadConfiguration("test_files/configuration/missing_eu5_directory.txt");
   EXPECT_THROW(configuration.Validate(commonItems::ConverterVersion()),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}


TEST(ConfigurationTest, ExceptionForBadEU5Directory)  // NOLINT : clang-tidy doens't like gtest
{
   const auto configuration = LoadConfiguration("test_files/configuration/bad_eu5_directory.txt");
   EXPECT_THROW(configuration.Validate(commonItems::ConverterVersion()),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}


TEST(ConfigurationTest, BadSaveNameThrowsException)  // NOLINT : clang-tidy doens't like gtest
{
   const auto configuration = LoadConfiguration("test_files/configuration/bad_save_name.txt");
   EXPECT_THROW(configuration.Validate(commonItems::ConverterVersion()),  // NOLINT : clang-tidy doens't like gtest
       std::invalid_argument);
}

TEST(ConfigurationTest, TooNewCk3VersionThrowsException)  // NOLINT : clang-tidy doens't like gtest
{
   commonItems::ConverterVersion converter_version;
   converter_version.loadVersion("test_files/version_old.txt");
   const auto configuration = LoadConfiguration("test_files/configuration/ck3_version.txt");
   EXPECT_THROW(configuration.Validate(converter_version),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}

TEST(ConfigurationTest, TooNewCk3VersionLogsError)  // NOLINT : clang-tidy doens't like gtest
{
   commonItems::ConverterVersion converter_version;
   const std::stringstream log;
   std::streambuf* cout_buffer = std::cout.rdbuf();
   std::cout.rdbuf(log.rdbuf());

   converter_version.loadVersion("test_files/version_old.txt");
   const auto configuration = LoadConfiguration("test_files/configuration/ck3_version.txt");
   EXPECT_THROW(configuration.Validate(converter_version),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
   EXPECT_THAT(log.str(), testing::HasSubstr(R"(CK3 version: 1.19.11.3)"));
   EXPECT_THAT(log.str(), testing::HasSubstr(R"(CK3 version is v1.19.11.3, converter requires maximum v1.5!)"));
   std::cout.rdbuf(cout_buffer);
}

TEST(ConfigurationTest, OldCk3VersionThrowsException)  // NOLINT : clang-tidy doens't like gtest
{
   commonItems::ConverterVersion converter_version;
   converter_version.loadVersion("test_files/version_new.txt");
   const auto configuration = LoadConfiguration("test_files/configuration/ck3_version.txt");
   EXPECT_THROW(configuration.Validate(converter_version),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}

TEST(ConfigurationTest, OldCk3VersionLogsError)  // NOLINT : clang-tidy doens't like gtest
{
   commonItems::ConverterVersion converter_version;
   const std::stringstream log;
   std::streambuf* cout_buffer = std::cout.rdbuf();
   std::cout.rdbuf(log.rdbuf());

   converter_version.loadVersion("test_files/version_new.txt");
   const auto configuration = LoadConfiguration("test_files/configuration/ck3_version.txt");

   EXPECT_THROW(configuration.Validate(converter_version),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
   EXPECT_THAT(log.str(), testing::HasSubstr(R"(CK3 version: 1.19.11.3)"));
   EXPECT_THAT(log.str(), testing::HasSubstr(R"(CK3 version is v1.19.11.3, converter requires minimum v2.5!)"));

   std::cout.rdbuf(cout_buffer);
}

TEST(ConfigurationTest, CorrectCk3VersionValidatesAndLogs)  // NOLINT : clang-tidy doens't like gtest
{
   commonItems::ConverterVersion converter_version;
   const std::stringstream log;
   std::streambuf* cout_buffer = std::cout.rdbuf();
   std::cout.rdbuf(log.rdbuf());

   converter_version.loadVersion("test_files/version.txt");
   const auto configuration = LoadConfiguration("test_files/configuration/ck3_version.txt");
   configuration.Validate(converter_version);

   EXPECT_THAT(log.str(),
       testing::HasSubstr(R"(Crusader Kings 3 install path is "test_files/test_folders/ck3_folder_with_version")"));
   EXPECT_THAT(log.str(), testing::HasSubstr(R"(CK3 version: 1.19.11.3)"));

   std::cout.rdbuf(cout_buffer);
}

TEST(ConfigurationTest, CorrectConfigurationValidatedWithoutErrors)  // NOLINT : clang-tidy doens't like gtest
{
   const auto configuration = LoadConfiguration("test_files/configuration/test_configuration.txt");
   configuration.Validate(commonItems::ConverterVersion());
}

}  // namespace configuration