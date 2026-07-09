#include <external/commonItems/external/googletest/googlemock/include/gmock/gmock-matchers.h>
#include <external/commonItems/external/googletest/googletest/include/gtest/gtest.h>

#include "src/configuration/configuration_loader.hpp"

// Tests copied from Vic3toEU4 - these aren't really unit tests, more like e2e tests, but they should suffice

namespace configuration
{


TEST(ConfigurationTest, DefaultsAreDefaulted)
{
   const auto configuration =
       LoadConfiguration("test_files/configuration/blank_configuration.txt", commonItems::ConverterVersion());

   EXPECT_TRUE(configuration.ck3_directory.empty());
   EXPECT_TRUE(configuration.ck3_doc_directory.empty());
   EXPECT_TRUE(configuration.eu5_directory.empty());
   EXPECT_TRUE(configuration.eu5_mod_path.empty());
   EXPECT_EQ(configuration.save_game,
       "test_save.ck3");  // a missing save would throw an exception, so this is set in the 'blank' config
   EXPECT_FALSE(configuration.debug);
   EXPECT_EQ(configuration.output_name, "test_save");  // if not specified, derived from the save
}


TEST(ConfigurationTest, ExceptionForMissingVic3Directory)
{
   EXPECT_THROW([[maybe_unused]] const auto _ = LoadConfiguration("test_files/configuration/missing_ck3_directory.txt",
                    commonItems::ConverterVersion()),
       std::runtime_error);
}


TEST(ConfigurationTest, ExceptionForBadVic3Directory)
{
   EXPECT_THROW([[maybe_unused]] const auto _ = LoadConfiguration("test_files/configuration/bad_ck3_directory.txt",
                    commonItems::ConverterVersion()),
       std::runtime_error);
}


TEST(ConfigurationTest, ExceptionForMissingHoI4Directory)
{
   EXPECT_THROW([[maybe_unused]] const auto _ = LoadConfiguration("test_files/configuration/missing_eu5_directory.txt",
                    commonItems::ConverterVersion()),
       std::runtime_error);
}


TEST(ConfigurationTest, ExceptionForBadHoI4Directory)
{
   EXPECT_THROW([[maybe_unused]] const auto _ = LoadConfiguration("test_files/configuration/bad_eu5_directory.txt",
                    commonItems::ConverterVersion()),
       std::runtime_error);
}


TEST(ConfigurationTest, ItemsCanBeImported)
{
   const auto configuration =
       LoadConfiguration("test_files/configuration/test_configuration.txt", commonItems::ConverterVersion());

   EXPECT_EQ(configuration.ck3_directory, R"(test_files/test_folders/ck3_folder)");
   EXPECT_EQ(configuration.ck3_doc_directory, "ck3_doc_directory");
   EXPECT_EQ(configuration.eu5_directory, R"(test_files/test_folders/eu5_folder)");
   EXPECT_EQ(configuration.eu5_mod_path, "eu5_mod_directory");
   EXPECT_EQ(configuration.save_game, "test_save.ck3");
   EXPECT_TRUE(configuration.debug);
   EXPECT_EQ(configuration.output_name, "test_output_name");
}


TEST(ConfigurationTest, ItemsAreLoggedWhenImported)
{
   std::stringstream log;
   std::streambuf* cout_buffer = std::cout.rdbuf();
   std::cout.rdbuf(log.rdbuf());

   const auto _ = LoadConfiguration("test_files/configuration/test_configuration.txt", commonItems::ConverterVersion());

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


TEST(ConfigurationTest, BadSaveNameThrowsException)
{
   EXPECT_THROW([[maybe_unused]] const auto _ =
                    LoadConfiguration("test_files/configuration/bad_save_name.txt", commonItems::ConverterVersion()),
       std::invalid_argument);
}


TEST(ConfigurationTest, OutputNameIsFromSave)
{
   const auto configuration = LoadConfiguration("test_files/configuration/output_name_from_save_configuration.txt",
       commonItems::ConverterVersion());

   EXPECT_EQ(configuration.output_name, "test_save_with_spaces");
}


TEST(ConfigurationTest, CustomOutputOverridesSaveOutputName)
{
   const auto configuration =
       LoadConfiguration("test_files/configuration/output_name_override.txt", commonItems::ConverterVersion());

   EXPECT_EQ(configuration.output_name, "path_has_not__been__removed__override_name_with_spaces");
}

}  // namespace configuration