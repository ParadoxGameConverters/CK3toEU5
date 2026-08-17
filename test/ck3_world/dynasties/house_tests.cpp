#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/dynasties/house.hpp"

TEST(CK3WorldHouseTests, HouseIDLoads)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::House house(input, 42);

   EXPECT_EQ(42, house.GetID());
}

TEST(CK3WorldHouseTests, LoadValuesDefaultToBlank)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::House house(input, 1);

   EXPECT_TRUE(house.GetKey().empty());
   EXPECT_TRUE(house.GetName().empty());
   EXPECT_TRUE(house.GetLocalizedName().empty());
   EXPECT_TRUE(house.GetPrefix().empty());
   EXPECT_EQ(house.GetDynasty().GetID(), -1);
}

TEST(CK3WorldHouseTests, HousePrimitivesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "key = \"house_vimaranes\"\n";
   input << "name = \"dynn_Villeneuve\"\n";
   input << "localized_name = \"Gaye\"\n";
   input << "prefix = \"dynnp_de\"\n";
   input << "dynasty = 19\n";

   const ck3::House house(input, 42);

   EXPECT_EQ("house_vimaranes", house.GetKey());
   EXPECT_EQ("dynn_Villeneuve", house.GetName());
   EXPECT_EQ("Gaye", house.GetLocalizedName());
   EXPECT_EQ("dynnp_de", house.GetPrefix());
   EXPECT_EQ(19, house.GetDynasty().GetID());
}
