#include <gmock/gmock-matchers.h>

#include <optional>
#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/cultures/culture.hpp"

TEST(CK3WorldCultureTests, CultureIDLoads)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Culture culture(input, 42);

   EXPECT_EQ(42, culture.GetID());
}

TEST(CK3WorldCultureTests, LoadValuesDefaultToDefaults)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Culture culture(input, 1);

   EXPECT_EQ(culture.GetName(), "noname");
   EXPECT_TRUE(culture.IsDynamic());
   EXPECT_TRUE(culture.GetHeritage().empty());
   EXPECT_TRUE(culture.GetEthos().empty());
   EXPECT_EQ(std::nullopt, culture.GetLocalizedName());
   EXPECT_EQ(std::nullopt, culture.GetTemplate());
   EXPECT_TRUE(culture.GetNameLists().empty());
   EXPECT_TRUE(culture.GetTraditions().empty());
}

TEST(CK3WorldCultureTests, CulturePrimitivesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "culture_template = akan\n";
   input << "name = \"Akan\"\n";
   input << "heritage = heritage_akan\n";
   input << "language = language_kru\n";
   input << "name_list = name_list_akan\n";
   input << "name_list = name_list_blahakan\n";
   input << "ethos = ethos_egalitarian\n";
   input << "traditions = { tradition_parochialism tradition_bush_hunting }\n";
   input << "culture_era_data={ {type = culture_era_tribal progress = 100}{type = culture_era_early_medieval progress "
            "= 59.85} {type = culture_era_high_medieval}{type = culture_era_late_medieval}}\n";

   const ck3::Culture culture(input, 42);

   EXPECT_EQ("akan", culture.GetTemplate());
   EXPECT_EQ("Akan", culture.GetLocalizedName());
   EXPECT_EQ("akan", culture.GetName());
   EXPECT_EQ("heritage_akan", culture.GetHeritage());
   EXPECT_EQ("language_kru", culture.GetLanguage());
   EXPECT_EQ("akan", culture.GetTemplate());
   EXPECT_EQ("ethos_egalitarian", culture.GetEthos());
   EXPECT_EQ("culture_era_tribal", culture.GetEra());
   EXPECT_THAT(culture.GetTraditions(), testing::ElementsAre("tradition_parochialism", "tradition_bush_hunting"));
   EXPECT_THAT(culture.GetNameLists(), testing::ElementsAre("akan", "blahakan"));
   EXPECT_FALSE(culture.IsDynamic());
}

TEST(CK3WorldCultureTests, CultureEraLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "culture_era_data={ {type = culture_era_tribal progress = 100 left=950}{type = culture_era_early_medieval "
            "progress "
            "= 100 join=950} {type = culture_era_high_medieval progress = 59.78}{type = culture_era_late_medieval}}\n";

   const ck3::Culture culture(input, 42);

   EXPECT_EQ("culture_era_early_medieval", culture.GetEra());
}

TEST(CK3WorldCultureTests, CultureNameLoadedForDynamic)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "name = \"Akano Hungarian\"\n";
   input << "heritage = heritage_akan\n";
   input << "name_list = name_list_akan\n";
   input << "name_list = name_list_blahakan\n";
   input << "ethos = ethos_egalitarian\n";
   input << "traditions = { tradition_parochialism tradition_bush_hunting }\n";
   input << "culture_era_data={ {type = culture_era_tribal progress = 100}{type = culture_era_early_medieval progress "
            "= 59.85} {type = culture_era_high_medieval}{type = culture_era_late_medieval}}\n";

   const ck3::Culture culture(input, 42);

   EXPECT_EQ("Akano Hungarian", culture.GetLocalizedName());
   EXPECT_EQ("Akano Hungarian", culture.GetName());
   EXPECT_EQ("heritage_akan", culture.GetHeritage());
   EXPECT_EQ(std::nullopt, culture.GetTemplate());
   EXPECT_EQ("ethos_egalitarian", culture.GetEthos());
   EXPECT_EQ("culture_era_tribal", culture.GetEra());
   EXPECT_TRUE(culture.IsDynamic());
   EXPECT_THAT(culture.GetTraditions(), testing::ElementsAre("tradition_parochialism", "tradition_bush_hunting"));
   EXPECT_THAT(culture.GetNameLists(), testing::ElementsAre("akan", "blahakan"));
}
