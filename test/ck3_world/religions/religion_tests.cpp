#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/religions/religion.hpp"

TEST(CK3WorldReligionTests, ReligionIDLoads)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Religion religion(input, 42);

   ASSERT_EQ(42, religion.GetID());
}

TEST(CK3WorldReligionTests, LoadValuesDefaultToBlank)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Religion religion(input, 1);

   ASSERT_TRUE(religion.GetTag().empty());
   ASSERT_TRUE(religion.GetFamily().empty());
   ASSERT_TRUE(religion.GetFaiths().empty());
   ASSERT_TRUE(religion.GetReligionType().empty());
}

TEST(CK3WorldReligionTests, ReligionPrimitivesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "tag = \"christianity_religion\"\n";
   input << "religion_type=zunism_religion\n";
   input << "family = \"rf_abrahamic\"\n";
   input << "faiths={ 9 10 11 12 13 }\n";

   const ck3::Religion religion(input, 42);

   ASSERT_EQ("christianity_religion", religion.GetTag());
   ASSERT_EQ("rf_abrahamic", religion.GetFamily());
   ASSERT_EQ("zunism_religion", religion.GetReligionType());
   ASSERT_EQ(5, religion.GetFaiths().size());
}
