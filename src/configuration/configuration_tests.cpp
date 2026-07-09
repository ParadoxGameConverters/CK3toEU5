#include <external/commonItems/external/googletest/googlemock/include/gmock/gmock-matchers.h>
#include <external/commonItems/external/googletest/googletest/include/gtest/gtest.h>

#include "src/configuration/configuration.hpp"
#include "src/configuration/configuration_loader.hpp"

// Tests copied from Vic3toEU4 - these aren't really unit tests, more like e2e tests, but they should suffice

namespace configuration
{


TEST(ConfigurationTest, ExceptionForMissingVic3Directory)
{
   const auto configuration = LoadConfiguration("test_files/configuration/missing_ck3_directory.txt");
   EXPECT_THROW(configuration.validate(commonItems::ConverterVersion()), std::runtime_error);
}


TEST(ConfigurationTest, ExceptionForBadVic3Directory)
{
   const auto configuration = LoadConfiguration("test_files/configuration/bad_ck3_directory.txt");
   EXPECT_THROW(configuration.validate(commonItems::ConverterVersion()), std::runtime_error);
}


TEST(ConfigurationTest, ExceptionForMissingHoI4Directory)
{
   const auto configuration = LoadConfiguration("test_files/configuration/missing_eu5_directory.txt");
   EXPECT_THROW(configuration.validate(commonItems::ConverterVersion()), std::runtime_error);
}


TEST(ConfigurationTest, ExceptionForBadHoI4Directory)
{
   const auto configuration = LoadConfiguration("test_files/configuration/bad_eu5_directory.txt");
   EXPECT_THROW(configuration.validate(commonItems::ConverterVersion()), std::runtime_error);
}


TEST(ConfigurationTest, BadSaveNameThrowsException)
{
   const auto configuration = LoadConfiguration("test_files/configuration/bad_save_name.txt");
   EXPECT_THROW(configuration.validate(commonItems::ConverterVersion()), std::runtime_error);
}

TEST(ConfigurationTest, CorrectConfigurationValidatedWithoutErrors)
{
   const auto configuration = LoadConfiguration("test_files/configuration/test_configuration.txt");
   configuration.validate(commonItems::ConverterVersion());
}

}  // namespace configuration