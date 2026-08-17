#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/dynasties/dynasty.hpp"

TEST(CK3WorldDynastyTests, DynastyIDLoads)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Dynasty dynasty(input, 42);

   ASSERT_EQ(42, dynasty.GetSavegameDynastyID());
}

TEST(CK3WorldDynastyTests, LoadValuesDefaultToBlank)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Dynasty dynasty(input, 1);

   ASSERT_TRUE(dynasty.GetDynastyID().empty());
   ASSERT_FALSE(dynasty.IsAppropriateRealmName());
}

TEST(CK3WorldDynastyTests, DynastyPrimitivesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "key = \"7\"\n";
   input << "good_for_realm_name = yes\n";
   input << "coat_of_arms_id = 14\n";

   const ck3::Dynasty dynasty(input, 42);

   ASSERT_EQ("7", dynasty.GetDynastyID());
   ASSERT_TRUE(dynasty.IsAppropriateRealmName());
}
