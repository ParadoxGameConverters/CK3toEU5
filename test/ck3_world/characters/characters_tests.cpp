#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/characters/characters.hpp"

namespace ck3
{


TEST(CK3WorldCharactersTests, CharactersDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   Characters characters;
   characters.ParseCharacters(input);

   ASSERT_TRUE(characters.GetAliveCharacters().empty());
}

TEST(CK3World_CharactersTests, CharactersCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "11={}\n";
   input << "13={first_name=\"bob\"}\n";
   input << "15={first_name=\"alice\"}\n";

   Characters characters;
   characters.ParseCharacters(input);
   const auto& char1 = characters.GetAliveCharacters().at(11);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& char2 = characters.GetAliveCharacters().at(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& char3 = characters.GetAliveCharacters().at(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_TRUE(char1->GetClaims().empty());
   ASSERT_EQ("bob", char2->GetName());
   ASSERT_EQ("alice", char3->GetName());
}

TEST(CK3World_CharactersTests, DeadCharactersCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "11={}\n";
   input << "13={first_name=\"bob\" dead_data={date = 31.8.26} }\n ";
   input << "15={first_name=\"alice\" dead_data={date = 772.8.26}}\n";

   Characters characters;
   characters.ParseCharacters(input);
   const auto& char2 = characters.GetDeadCharacters().at(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& char3 = characters.GetDeadCharacters().at(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_FALSE(characters.GetDeadCharacters().contains(11));  // NOLINT(readability-magic-numbers) : "magic number"
   ASSERT_EQ("bob", char2->GetName());
   ASSERT_EQ("alice", char3->GetName());
}

}  // namespace ck3
