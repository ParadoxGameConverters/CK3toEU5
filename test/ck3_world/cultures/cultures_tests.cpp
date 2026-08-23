#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/cultures/cultures.hpp"

TEST(CK3WorldCulturesTests, CulturesDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Cultures cultures(input);

   EXPECT_TRUE(cultures.GetCultures().empty());
}

TEST(CK3WorldCulturesTests, BundledCulturesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "cultures={\n";
   input << "\t13={culture_template=\"akan\"}\n";
   input << "\t15={culture_template=\"kru\"}\n";
   input << "}\n";

   const ck3::Cultures cultures(input);
   const auto& culture1 = cultures.GetCultures().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& culture2 = cultures.GetCultures().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   EXPECT_EQ(2, cultures.GetCultures().size());
   EXPECT_EQ("akan", culture1->second->GetTemplate());
   EXPECT_EQ("kru", culture2->second->GetTemplate());
}
