#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/dynasties/dynasties.hpp"

TEST(CK3WorldDynastiesTests, DynastiesDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Dynasties dynasties(input);

   ASSERT_TRUE(dynasties.GetDynasties().empty());
}

TEST(CK3WorldDynastiesTests, DynastiesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "dynasties={\n";
   input << "\t13={key=\"2\"}\n";
   input << "\t15={key=\"7\"}\n";
   input << "}";

   const ck3::Dynasties dynasties(input);
   const auto& dynasty1 = dynasties.GetDynasties().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& dynasty2 = dynasties.GetDynasties().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ(2, dynasties.GetDynasties().size());
   ASSERT_EQ("2", dynasty1->second->GetDynastyID());
   ASSERT_EQ("7", dynasty2->second->GetDynastyID());
}

TEST(CK3WorldDynastiesTests, NonsenseDynastiesAreIgnored)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "dynasties={\n";
   input << "\t13=none\n";
   input << "\t15={key=\"7\"}\n";
   input << "}";

   const ck3::Dynasties dynasties(input);
   const auto& dynasty1 = dynasties.GetDynasties().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& dynasty2 = dynasties.GetDynasties().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ(1, dynasties.GetDynasties().size());
   ASSERT_EQ(dynasties.GetDynasties().end(), dynasty1);
   ASSERT_EQ("7", dynasty2->second->GetDynastyID());
}


TEST(CK3WorldDynastiesTests, HousesDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Dynasties houses(input);

   EXPECT_TRUE(houses.GetHouses().empty());
}

TEST(CK3WorldDynastiesTests, HousesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "dynasty_house={\n";
   input << "13={name=\"dynn_Villeneuve\"}\n";
   input << "15={name=\"dynn_Fournier\"}\n";
   input << "}";

   const ck3::Dynasties houses(input);
   const auto& house1 = houses.GetHouses().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& house2 = houses.GetHouses().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   EXPECT_EQ("dynn_Villeneuve", house1->second->GetName());
   EXPECT_EQ("dynn_Fournier", house2->second->GetName());
}


TEST(CK3WorldDynastiesTests, NonsenseHousesAreIgnored)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "dynasty_house={\n";
   input << "13=none\n";
   input << "15={name=\"dynn_Fournier\"}\n";
   input << "}";

   const ck3::Dynasties houses(input);
   const auto& house1 = houses.GetHouses().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& house2 = houses.GetHouses().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   EXPECT_EQ(1, houses.GetHouses().size());
   EXPECT_EQ(houses.GetHouses().end(), house1);
   EXPECT_EQ("dynn_Fournier", house2->second->GetName());
}