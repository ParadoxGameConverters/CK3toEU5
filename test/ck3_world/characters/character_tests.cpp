#include <iostream>
#include <optional>
#include <sstream>

#include "Date.h"
#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"
#include "src/ck3_world/characters/character.hpp"

namespace ck3
{

TEST(CK3WorldCharacterTests, CharacterIDLoads)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const Character character(input, 42);

   ASSERT_EQ(42, character.GetID());
}

TEST(CK3WorldCharacterTests, LoadValuesDefaultToBlank)  // NOLINT - readability-function-cognitive-complexity
{
   std::stringstream input;
   ck3::Character character(input, 1);

   ASSERT_TRUE(character.GetName().empty());
   ASSERT_EQ(date("1.1.1"), character.GetBirthDate());
   ASSERT_EQ(std::nullopt, character.GetCulture());
   ASSERT_EQ(std::nullopt, character.GetFaith());
   ASSERT_EQ(character.GetHouse().GetID(), -1);
   ASSERT_FALSE(character.GetSkills().martial);
   ASSERT_FALSE(character.GetSkills().diplomacy);
   ASSERT_FALSE(character.GetSkills().stewardship);
   ASSERT_FALSE(character.GetSkills().learning);
   ASSERT_FALSE(character.GetSkills().intrigue);
   ASSERT_TRUE(character.GetTraits().empty());
   ASSERT_EQ(0.0F, character.GetPiety());
   ASSERT_EQ(std::nullopt, character.GetPrestige());
   ASSERT_EQ(0.0F, character.GetGold());
   ASSERT_TRUE(character.GetClaims().empty());
   ASSERT_EQ(std::nullopt, character.GetEmployer());
   ASSERT_FALSE(character.IsKnight());
   ASSERT_FALSE(character.IsFemale());
   ASSERT_FALSE(character.IsCouncilor());
   ASSERT_EQ(std::nullopt, character.GetSpouse());
   ASSERT_EQ(std::nullopt, character.GetCharacterRealm());
   ASSERT_FALSE(character.IsDead());
}

TEST(CK3WorldCharacterTests, CharacterPrimitivesCanBeLoaded)  // NOLINT - readability-function-cognitive-complexity
{
   std::stringstream input;
   input << "first_name = \"bob spongepants\"\n";
   input << "birth = 9.9.9\n";
   input << "culture = 1\n";
   input << "faith = 2\n";
   input << "dynasty_house = 3\n";
   input << "skill = { 11 12 13 14 15 16 }\n";
   input << "traits = { 21 22 23 24 25 26 26 26 }\n";
   input << "female = yes\n";
   input << "dead_data = { date=31.8.26 }\n";

   Character character(input, 42);

   ASSERT_EQ("bob spongepants", character.GetName());
   ASSERT_EQ(date("9.9.9"), character.GetBirthDate());
   ASSERT_TRUE(character.GetCulture().has_value());
   ASSERT_EQ(1, character.GetCulture()->GetID());  // NOLINT : bugprone-unchecked-optional-access
   ASSERT_TRUE(character.GetFaith().has_value());
   ASSERT_EQ(2, character.GetFaith()->GetID());  // NOLINT : bugprone-unchecked-optional-access
   ASSERT_EQ(3, character.GetHouse().GetID());
   ASSERT_EQ(11, character.GetSkills().diplomacy);
   ASSERT_EQ(12, character.GetSkills().martial);
   ASSERT_EQ(13, character.GetSkills().stewardship);
   ASSERT_EQ(14, character.GetSkills().intrigue);
   ASSERT_EQ(15, character.GetSkills().learning);
   ASSERT_EQ(6, character.GetTraits().size());
   ASSERT_EQ(1, character.GetTraits().count(26));
   ASSERT_EQ(1, character.GetTraits().count(21));
   ASSERT_TRUE(character.IsFemale());
   ASSERT_TRUE(character.IsDead());
   ASSERT_EQ(character.GetDeathDate(), date(31, 8, 26));
}

TEST(CK3WorldCharacterTests, MalformedCharacterSkillsPrintsError)  // NOLINT - readability-function-cognitive-complexity
{
   const std::stringstream log;
   std::streambuf* cout_buffer = std::cout.rdbuf();
   std::cout.rdbuf(log.rdbuf());

   std::stringstream input;
   input << "first_name = \"bob spongepants\"\n";
   input << "birth = 9.9.9\n";
   input << "skill = { 11 12 13 14 15 16 90 }\n";

   const Character character(input, 42);

   ASSERT_EQ("bob spongepants", character.GetName());
   ASSERT_EQ(date("9.9.9"), character.GetBirthDate());
   EXPECT_THAT(log.str(), testing::HasSubstr(R"(Character 42 has a malformed skills block! Size: 7)"));

   std::cout.rdbuf(cout_buffer);
}

TEST(CK3WorldCharacterTests, CharacterAliveDataCanBeLoaded)  // NOLINT - readability-function-cognitive-complexity
{
   std::stringstream input;
   input << "alive_data = {\n";
   input << "\tpiety = {\n";
   input << "\t\taccumulated = 100.01\n";
   input << "\t}\n";
   input << "\tprestige = {\n";
   input << "\t\taccumulated = 101.02\n";
   input << "\t}\n";
   input << "\tinfluence = {\n";
   input << "\t\taccumulated = 101.02\n";
   input << "\t}\n";
   input << "\tmerit = {\n";
   input << "\t\taccumulated = 101.02\n";
   input << "\t}\n";
   input << "\tgold = {\n";
   input << "\t\tvalue = 103.02\n";
   input << "\t}\n";
   input << "\tclaim = { { title = 1 } { title = 3 } { title = 5 } }\n";
   input << "obedience_target=336\n";
   input << "is_obedient=no\n";
   input << "laws={ japanese_bureaucracy_2 single_heir_succession_law }\n";
   input << "realm_capital = 13414\n";
   input << "domain={ 13176 13177 13297}\n";
   input << "government=japan_feudal_government\n";
   input << "}";

   Character character(input, 42);

   ASSERT_NEAR(100.01, character.GetPiety(), 0.001);
   ASSERT_TRUE(character.GetPrestige().has_value());
   ASSERT_NEAR(101.02, character.GetPrestige().value_or(0), 0.001);
   ASSERT_NEAR(101.02, character.GetMerit().value_or(0), 0.001);
   ASSERT_NEAR(101.02, character.GetInfluence().value_or(0), 0.001);
   ASSERT_NEAR(103.02, character.GetGold(), 0.001);
   ASSERT_EQ(3, character.GetClaims().size());
   ASSERT_EQ(336, character.GetSuzerain()->GetID());  // NOLINT
}

TEST(CK3WorldCharacterTests, CharacterRealmCanBeLoaded)  // NOLINT - readability-function-cognitive-complexity
{
   std::stringstream input;
   input << "landed_data={\n";
   input << "laws={ japanese_bureaucracy_2 single_heir_succession_law }\n";
   input << "realm_capital = 13414\n";
   input << "domain={ 13176 13177 13297}\n";
   input << "government=japan_feudal_government\n";
   input << "}\n";

   Character character(input, 42);

   ASSERT_TRUE(character.GetCharacterRealm().has_value());
}


TEST(CK3WorldCharacterTests, CharacterFamilyDataCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "family_data = {\n";
   input << "\tprimary_spouse = 17\n";
   input << "child={ 90 78 }\n";
   input << "spouse = 56\n";
   input << "concubine = 101\n";
   input << "concubine = 103\n";
   input << "}";

   Character character(input, 42);

   ASSERT_TRUE(character.GetSpouse().has_value());
   // ASSERT_EQ(17, character.GetSpouse()->GetID());  // NOLINT : bugprone-unchecked-optional-access
   ASSERT_EQ(character.GetChildren().size(), 2);
   ASSERT_EQ(character.GetChildren()[0].GetID(), 90);
   ASSERT_EQ(character.GetChildren()[1].GetID(), 78);
   ASSERT_EQ(character.GetConcubines().size(), 2);
   ASSERT_EQ(character.GetConcubines()[0].GetID(), 101);
   ASSERT_EQ(character.GetConcubines()[1].GetID(), 103);
}

TEST(CK3WorldCharacterTests, CharacterPlayableDataCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "playable_data = \n"
            "{ knights = {128103 33670375 124029 50395737 102236 117863 67190927}\n"
            "diarchy = 973078556\n"
            "diarchy_successor = 134276006\n"
            "accolades = {67109158 33554852 50332246}\n"
            "legitimacy=567.92187\n"
            "}\n";

   Character character(input, 42);

   ASSERT_EQ(character.GetKnights().size(), 7);
   ASSERT_EQ(character.GetKnights()[0].GetID(), 128103);
   ASSERT_TRUE(character.GetLegitimacy().has_value());
   ASSERT_NEAR(567.92187, character.GetLegitimacy().value_or(0), 0.001);
}

TEST(CK3WorldCharacterTests, CharacterCourtDataCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "court_data = {\n";
   input << "\temployer = 27\n";
   input << "\tknight = yes\n";
   input << "\tcouncil_task = 7248\n";
   input << "}";

   Character character(input, 42);

   ASSERT_TRUE(character.GetEmployer().has_value());
   ASSERT_EQ(27, character.GetEmployer().value().GetID());  // NOLINT : bugprone-unchecked-optional-access
   ASSERT_TRUE(character.IsKnight());
   ASSERT_TRUE(character.IsCouncilor());
}

}  // namespace ck3