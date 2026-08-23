#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/confederations/confederation.hpp"
#include "src/ck3_world/id_pointer_pair.hpp"


TEST(CK3WorldConfederationTests, CultureIDLoads)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Confederation confederation(input, 42);

   EXPECT_EQ(42, confederation.GetID());
}

TEST(CK3WorldConfederationTests, LoadValuesDefaultToDefaults)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Confederation confederation(input, 1);

   EXPECT_EQ(1, confederation.GetID());
   EXPECT_TRUE(confederation.GetName().empty());
   EXPECT_TRUE(confederation.GetHouses().empty());
   EXPECT_TRUE(confederation.GetMembers().empty());
}

TEST(CK3WorldConfederationTests, ConfederationPrimitivesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "members={ 16793418 16795222 16810363 26726 16799741 }\n";
   input << "houses={ 1679 167222 16810 }\n";
   input << "leader = 610\n";
   input << "name=\"Polabian Confederation\"\n";
   input << "color=rgb { 80 110 80 }\n";
   input << "coat_of_arms=20636\n";

   const ck3::Confederation confederation(input, 42);

   EXPECT_EQ(confederation.GetMembers().size(), 5);
   EXPECT_EQ(confederation.GetMembers()[0].GetID(), 16793418);

   EXPECT_EQ(confederation.GetHouses().size(), 3);
   EXPECT_EQ(confederation.GetHouses()[0].GetID(), 1679);

   EXPECT_EQ("Polabian Confederation", confederation.GetName());
   EXPECT_EQ(610, confederation.GetLeaderHouse().GetID());
}
