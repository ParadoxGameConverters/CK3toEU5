#include <sstream>
#include <stdexcept>

#include "gtest/gtest.h"
#include "src/ck3_world/characters/characters.hpp"
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

TEST(CK3WorldDynastiesTests, CharactersCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "dynasties={\n";
   input << "13=none\n";
   input << "15={name=\"dynn_Fournier\" key=8 dynasty_head=1}\n";
   input << "}";
   ck3::Dynasties dynasties(input);

   std::stringstream input2;
   input2 << "1={first_name=mieczyslaw}\n";
   input2 << "2={first_name=wieczyslaw}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   dynasties.LinkCharacters(characters);

   const auto& dynasty1 = dynasties.GetDynasties().find(15);  // NOLINT(readability-magic-numbers)

   ASSERT_TRUE(dynasty1->second->GetDynastyHead().has_value());
   ASSERT_EQ("mieczyslaw",
       dynasty1  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetDynastyHead()
           ->GetPointer()
           .lock()
           ->GetName());
}

TEST(CK3WorldDynastiesTests, LinkingMissingDynastyHeadThrows)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "dynasties={\n";
   input << "13=none\n";
   input << "15={name=\"dynn_Fournier\" key=8 dynasty_head=6}\n";
   input << "}";
   ck3::Dynasties dynasties(input);

   std::stringstream input2;
   input2 << "1={first_name=mieczyslaw}\n";
   input2 << "2={first_name=wieczyslaw}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   ASSERT_THROW(dynasties.LinkCharacters(characters),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}

TEST(CK3WorldDynastiesTests, HouseHeadsCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "dynasty_house={\n";
   input << "13=none\n";
   input << "15={name=\"dynn_Fournier\" head_of_house=1}\n";
   input << "}";
   ck3::Dynasties dynasties(input);

   std::stringstream input2;
   input2 << "1={first_name=mieczyslaw}\n";
   input2 << "2={first_name=wieczyslaw}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   dynasties.LinkCharacters(characters);

   const auto& house1 = dynasties.GetHouses().find(15);  // NOLINT(readability-magic-numbers)

   ASSERT_TRUE(house1->second->GetHouseHead().has_value());
   ASSERT_EQ("mieczyslaw",
       house1  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetHouseHead()
           ->GetPointer()
           .lock()
           ->GetName());
}

TEST(CK3WorldDynastiesTests, LinkingMissingHouseHeadThrows)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "dynasty_house={\n";
   input << "13=none\n";
   input << "15={name=\"dynn_Fournier\" head_of_house=6}\n";
   input << "}";
   ck3::Dynasties dynasties(input);

   std::stringstream input2;
   input2 << "1={first_name=mieczyslaw}\n";
   input2 << "2={first_name=wieczyslaw}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   ASSERT_THROW(dynasties.LinkCharacters(characters),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}

TEST(CK3WorldDynastiesTests, HouseCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "dynasty_house={\n";
   input << "11=none\n";
   input << "12={name=\"dynn_Fournier\" head_of_house=1 dynasty=15}\n";
   input << "}";
   input << "dynasties={\n";
   input << "13=none\n";
   input << "15={name=\"dynn_Fournier\" key=87 dynasty_head=2}\n";
   input << "}";
   ck3::Dynasties dynasties(input);

   dynasties.LinkDynasties();

   const auto& house1 = dynasties.GetHouses().find(12);  // NOLINT(readability-magic-numbers)

   ASSERT_EQ("87",
       house1  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetDynasty()
           .GetPointer()
           .lock()
           ->GetDynastyID());
}

TEST(CK3WorldDynastiesTests, LinkingMissingDynastyThrows)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "dynasty_house={\n";
   input << "11=none\n";
   input << "12={name=\"dynn_Fournier\" head_of_house=1 dynasty=142}\n";
   input << "}";
   input << "dynasties={\n";
   input << "13=none\n";
   input << "15={name=\"dynn_Fournier\" key=87 dynasty_head=2}\n";
   input << "}";
   ck3::Dynasties dynasties(input);

   ASSERT_THROW(dynasties.LinkDynasties(),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}