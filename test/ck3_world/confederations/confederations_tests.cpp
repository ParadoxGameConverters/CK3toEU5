#include <sstream>
#include <stdexcept>

#include "gtest/gtest.h"
#include "src/ck3_world/characters/characters.hpp"
#include "src/ck3_world/confederations/confederations.hpp"
#include "src/ck3_world/dynasties/dynasties.hpp"


TEST(CK3WorldConfederationsTests, ConfederationsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Confederations confederations(input);

   EXPECT_TRUE(confederations.GetConfederations().empty());
}

TEST(CK3WorldConfederationsTests, ConfederationsCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;

   input << "database={\n";
   input << "		0={\n";
   input << "			members={ 16787777 9525 9944 33566183 16795849 16818520 }\n";
   input << "			name=\"Irish Confederation\"\n";
   input << "			color=rgb { 49 249 72 }\n";
   input << "			coat_of_arms=17763\n";
   input << "			variables={\n";
   input << "				data={ {\n";
   input << "						flag=confederation_culture\n";
   input << "						data={\n";
   input << "							type=culture\n";
   input << "							identity=65\n";
   input << "						}\n";
   input << "					}\n";
   input << "}\n";
   input << "			}\n";
   input << "		}\n";
   input << "		16777217={\n";
   input << "			members={ 39303 26333 33589596 10062 40945 16807097 46100 50346833 }\n";
   input << "			name=\"Khazar Confederation\"\n";
   input << "			color=rgb { 210 160 69 }\n";
   input << "			coat_of_arms=31944\n";
   input << "			variables={\n";
   input << "				data={ {\n";
   input << "						flag=confederation_culture\n";
   input << "						data={\n";
   input << "							type=culture\n";
   input << "							identity=157\n";
   input << "						}\n";
   input << "					}\n";
   input << " }\n";
   input << "			}\n";
   input << "}\n";
   input << "		16777218=none\n";
   input << "}\n";

   const ck3::Confederations confederations(input);
   const auto& confederation1 = confederations.GetConfederations().find(0);
   const auto& confederation2 = confederations.GetConfederations().find(16777217);  // NOLINT(readability-magic-numbers)

   EXPECT_EQ(2, confederations.GetConfederations().size());
   EXPECT_EQ("Irish Confederation", confederation1->second->GetName());
   EXPECT_EQ("Khazar Confederation", confederation2->second->GetName());
}

TEST(CK3WorldConfederationsTests, CharactersCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "database={\n";
   input << "13={leader=2 houses={5237 10802 10810 10817 10823} }\n ";
   input << "15={members={ 1 2 }}\n";
   input << "17={members={ 2 }}\n";
   input << "}\n";
   ck3::Confederations confederations(input);

   std::stringstream input2;
   input2 << "1={first_name=mieczyslaw}\n";
   input2 << "2={first_name=wieczyslaw}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   confederations.LinkCharacters(characters);

   const auto& confederation1 = confederations.GetConfederations().find(15);  // NOLINT(readability-magic-numbers)

   ASSERT_FALSE(confederation1->second->GetLeaderHouse().has_value());
   ASSERT_EQ("mieczyslaw",
       confederation1  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetMembers()[0]
           .GetPointer()
           .lock()
           ->GetName());
   ASSERT_EQ("wieczyslaw",
       confederation1  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetMembers()[1]
           .GetPointer()
           .lock()
           ->GetName());
}

TEST(CK3WorldConfederationsTests, LinkingMissingCharacterThrows)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "database={\n";
   input << "13={leader=2 houses={5237 10802 10810 10817 10823} }\n ";
   input << "15={members={ 1 2 }}\n";
   input << "17={members={ 2 6 }}\n";  // 6 is missing
   input << "}\n";
   ck3::Confederations confederations(input);

   std::stringstream input2;
   input2 << "1={first_name=mieczyslaw}\n";
   input2 << "2={first_name=wieczyslaw}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   ASSERT_THROW(confederations.LinkCharacters(characters),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}

TEST(CK3WorldConfederationsTests, HousesCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "database={\n";
   input << "13={leader=12 houses={11 12} }\n ";
   input << "15={members={ 1 2 }}\n";
   input << "17={members={ 2 }}\n";
   input << "}\n";
   ck3::Confederations confederations(input);

   std::stringstream input2;
   input2 << "dynasty_house={\n";
   input2 << "11={name=\"dynn_Villeneuve\"}\n";
   input2 << "12={name=\"dynn_Fournier\"}\n";
   input2 << "}";
   const ck3::Dynasties dynasties(input2);

   confederations.LinkHouses(dynasties);

   const auto& confederation2 = confederations.GetConfederations().find(13);  // NOLINT(readability-magic-numbers)

   ASSERT_EQ("dynn_Fournier",
       confederation2  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetLeaderHouse()
           ->GetPointer()
           .lock()
           ->GetName());

   ASSERT_EQ("dynn_Villeneuve",
       confederation2  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetHouses()[0]
           .GetPointer()
           .lock()
           ->GetName());
}

TEST(CK3WorldConfederationsTests, LinkingMissingHouseThrows)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "database={\n";
   input << "13={leader=11 houses={ 11 6} }\n ";  // 6 is missing
   input << "15={members={ 1 2 }}\n";
   input << "17={members={ 2 }}\n";
   input << "}\n";
   ck3::Confederations confederations(input);

   std::stringstream input2;
   input2 << "dynasty_house={\n";
   input2 << "11={name=\"dynn_Villeneuve\"}\n";
   input2 << "12={name=\"dynn_Fournier\"}\n";
   input2 << "}";
   const ck3::Dynasties dynasties(input2);

   ASSERT_THROW(confederations.LinkHouses(dynasties),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}

TEST(CK3WorldConfederationsTests, LinkingMissingLeaderHouseThrows)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "database={\n";
   input << "13={leader=6 houses={ 11 } }\n ";  // 6 is missing
   input << "15={members={ 1 2 }}\n";
   input << "17={members={ 2 }}\n";
   input << "}\n";
   ck3::Confederations confederations(input);

   std::stringstream input2;
   input2 << "dynasty_house={\n";
   input2 << "11={name=\"dynn_Villeneuve\"}\n";
   input2 << "12={name=\"dynn_Fournier\"}\n";
   input2 << "}";
   const ck3::Dynasties dynasties(input2);

   ASSERT_THROW(confederations.LinkHouses(dynasties),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}