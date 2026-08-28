#include <iostream>
#include <sstream>
#include <stdexcept>

#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"
#include "src/ck3_world/characters/characters.hpp"
#include "src/ck3_world/council_manager/councillor_tasks.hpp"
#include "src/ck3_world/cultures/cultures.hpp"
#include "src/ck3_world/dynasties/dynasties.hpp"
#include "src/ck3_world/religions/religions.hpp"
#include "src/ck3_world/titles/title.hpp"
#include "src/ck3_world/titles/titles.hpp"

namespace ck3
{


TEST(CK3WorldCharactersTests, CharactersDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   Characters characters;
   characters.ParseCharacters(input);

   ASSERT_TRUE(characters.GetAliveCharacters().empty());
}

TEST(CK3WorldCharactersTests, CharactersCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
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

   const auto& char2_from_all =
       characters.GetAllCharacters().at(13);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_TRUE(char1->GetClaims().empty());
   ASSERT_EQ("bob", char2->GetName());
   ASSERT_EQ("alice", char3->GetName());

   ASSERT_EQ("bob", char2_from_all->GetName());
}

TEST(CK3WorldCharactersTests, DeadCharactersCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
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

TEST(CK3WorldCharactersTests, CulturesCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "cultures={\n";
   input << "13={culture_template=\"akan\"}\n";
   input << "15={culture_template=\"kru\"}\n";
   input << "}\n";
   const ck3::Cultures cultures(input);

   std::stringstream input2;
   input2 << "1={culture = 15}\n";
   input2 << "2={culture = 13}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);
   characters.LinkCultures(cultures);

   const auto& character1 = characters.GetAliveCharacters().find(1);
   const auto& character2 = characters.GetAliveCharacters().find(2);

   ASSERT_EQ("kru",
       character1  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetCulture()
           ->GetPointer()
           .lock()
           ->GetTemplate());
   ASSERT_EQ("akan",
       character2  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetCulture()
           ->GetPointer()
           .lock()
           ->GetTemplate());
}

TEST(CK3WorldCharactersTests, LinkingMissingCultureThrowsException)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "cultures={\n";
   input << "13={culture_template=\"akan\"}\n";
   input << "}\n";
   const ck3::Cultures cultures(input);

   std::stringstream input2;
   input2 << "1={culture = 13}\n";
   input2 << "2={culture = 15}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   ASSERT_THROW(characters.LinkCultures(cultures), std::runtime_error);  // NOLINT : clang-tidy doens't like gtest
}

TEST(CK3WorldCharactersTests, LinkingCharacterWithoutCultureDoesntThrow)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Cultures cultures(input);

   std::stringstream input2;
   input2 << "1={first_name=zabak}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   characters.LinkCultures(cultures);
}

TEST(CK3WorldCharactersTests, FaithsCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "faiths={\n";
   input << "13={tag=\"old_bon\"}\n";
   input << "15={tag=\"theravada\"}\n";
   input << "}\n";
   const ck3::Religions religions(input);

   std::stringstream input2;
   input2 << "1={faith = 15}\n";
   input2 << "2={faith = 13}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);
   characters.LinkFaiths(religions);

   const auto& character1 = characters.GetAliveCharacters().find(1);
   const auto& character2 = characters.GetAliveCharacters().find(2);

   ASSERT_EQ("theravada",
       character1->second->GetFaith()->GetPointer().lock()->GetTag());  // NOLINT(bugprone-unchecked-optional-access)
   ASSERT_EQ("old_bon",
       character2->second->GetFaith()->GetPointer().lock()->GetTag());  // NOLINT(bugprone-unchecked-optional-access)
}

TEST(CK3WorldCharactersTests, LinkingMissingFaithThrowsException)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "faiths={\n";
   input << "13={tag=\"old_bon\"}\n";
   input << "}\n";
   const ck3::Religions religions(input);

   std::stringstream input2;
   input2 << "1={faith = 15}\n";
   input2 << "2={faith = 13}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   ASSERT_THROW(characters.LinkFaiths(religions), std::runtime_error);  // NOLINT : clang-tidy doens't like gtest
}

TEST(CK3WorldCharactersTests, LinkingCharacterWithoutFaithDoesntThrow)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Religions religions(input);

   std::stringstream input2;
   input2 << "1={first_name=zabak}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   characters.LinkFaiths(religions);
}

TEST(CK3WorldCharactersTests, HousesCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "dynasty_house={\n";
   input << "13={name=\"dynn_Villeneuve\"}\n";
   input << "15={name=\"dynn_Fournier\"}\n";
   input << "}\n";
   const ck3::Dynasties dynasties(input);

   std::stringstream input2;
   input2 << "1={dynasty_house = 13}\n";
   input2 << "2={dynasty_house = 15}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);
   characters.LinkHouses(dynasties);

   const auto& character1 = characters.GetAliveCharacters().find(1);
   const auto& character2 = characters.GetAliveCharacters().find(2);

   ASSERT_EQ("dynn_Villeneuve",
       character1->second->GetHouse()->GetPointer().lock()->GetName());  // NOLINT(bugprone-unchecked-optional-access)
   ASSERT_EQ("dynn_Fournier",
       character2->second->GetHouse()->GetPointer().lock()->GetName());  // NOLINT(bugprone-unchecked-optional-access)
}

TEST(CK3WorldCharactersTests, LinkingMissingHouseThrowsException)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "dynasty_house={\n";
   input << "13={name=\"dynn_Villeneuve\"}\n";
   input << "}\n";
   const ck3::Dynasties dynasties(input);

   std::stringstream input2;
   input2 << "1={dynasty_house = 13}\n";
   input2 << "2={dynasty_house = 15}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   ASSERT_THROW(characters.LinkHouses(dynasties), std::runtime_error);  // NOLINT : clang-tidy doens't like gtest
}

TEST(CK3WorldCharactersTests, LinkingCharacterWithoutHouseDoesntThrow)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Dynasties dynasties(input);

   std::stringstream input2;
   input2 << "1={first_name=zabak}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   characters.LinkHouses(dynasties);
}

TEST(CK3WorldCharactersTests, TitlesCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "landed_titles={\n";
   input << "1 = { key=c_county1 }\n";
   input << "2 = { key=c_county2 }\n";
   input << "3 = { key=c_county3 }\n";
   input << "4 = { key=c_county4 }\n";
   input << "5 = { key=c_county5 }\n";
   input << "}";
   const ck3::Titles titles(input);

   std::stringstream input3;
   input3 << "active={\n";
   input3 << "101 = { type=task_foreign_affairs   owner=123   court_owner = 100 }\n";
   input3 << "103 = { type=task_foreign_affairs   owner=124   court_owner = 100 }\n";
   input3 << "}\n";
   const ck3::CouncillorTasks councillor_tasks(input3);

   std::stringstream input2;
   input2 << "100 = {\n";
   input2 << "alive_data={\n";
   input2 << "\tclaim = { { title = 1 } { title = 3 } { title = 5 } }\n";
   input2 << "}\n";
   input2 << "\tlanded_data = {\n";
   input2 << "\t\trealm_capital = 2\n";
   input2 << "\t\tdomain = { 4 2 }";
   input2 << "\t\tcouncil={ 101 103 }\n";
   input2 << "\t}";
   input2 << "}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);
   characters.LinkTitles(titles, councillor_tasks);

   const auto& character =
       characters.GetAliveCharacters().find(100);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ(3, character->second->GetClaims().size());
   ASSERT_EQ("c_county1", character->second->GetClaims().at(0).GetPointer().lock()->GetKey());
   ASSERT_EQ("c_county3", character->second->GetClaims().at(1).GetPointer().lock()->GetKey());
   ASSERT_EQ("c_county5", character->second->GetClaims().at(2).GetPointer().lock()->GetKey());
   ASSERT_EQ("c_county2",
       character  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetCharacterRealm()
           ->GetRealmCapital()
           ->GetPointer()
           .lock()
           ->GetKey());
   ASSERT_EQ(2,
       character->second->GetCharacterRealm()->GetDomain().size());  // NOLINT(bugprone-unchecked-optional-access)
   ASSERT_EQ("c_county4",
       character  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetCharacterRealm()
           ->GetDomain()
           .at(0)
           .GetPointer()
           .lock()
           ->GetKey());
   ASSERT_EQ("c_county2",
       character  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetCharacterRealm()
           ->GetDomain()
           .at(1)
           .GetPointer()
           .lock()
           ->GetKey());
}

TEST(CK3WorldCharactersTests, TitlesLinkMissingClaimDropsErrantClaim)  // NOLINT : clang-tidy doens't like gtest
{
   const std::stringstream log;
   std::streambuf* cout_buffer = std::cout.rdbuf();
   std::cout.rdbuf(log.rdbuf());

   std::stringstream input;
   input << "landed_titles={\n";
   input << "1 = { key=c_county1 }\n";
   input << "2 = { key=c_county2 }\n";
   input << "}";
   const ck3::Titles titles(input);

   std::stringstream input3;
   input3 << "active={\n";
   input3 << "}\n";
   const ck3::CouncillorTasks councillor_tasks(input3);

   std::stringstream input2;
   input2 << "100 = {\n";
   input2 << "first_name=zaba\n";
   input2 << "alive_data={\n";
   input2 << "\tclaim = { { title = 1 } { title = 2 } { title = 6 } }\n";  // title 6 is missing
   input2 << "}\n";
   input2 << "\tlanded_data = {\n";
   input2 << "\t\trealm_capital = 2\n";
   input2 << "\t\tdomain = { 1 2 }";
   input2 << "\t}";
   input2 << "}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);
   characters.LinkTitles(titles, councillor_tasks);

   const auto& character =
       characters.GetAliveCharacters().find(100);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ(2, character->second->GetClaims().size());
   ASSERT_EQ("c_county1", character->second->GetClaims().at(0).GetPointer().lock()->GetKey());
   ASSERT_EQ("c_county2", character->second->GetClaims().at(1).GetPointer().lock()->GetKey());

   EXPECT_THAT(log.str(),
       testing::HasSubstr(R"(Character 100 zaba has claim 6 which has no definition, removing it.)"));

   std::cout.rdbuf(cout_buffer);
}

TEST(CK3WorldCharactersTests, CharactersCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "1 = {\n";
   input << "\tfirst_name = Alice\n";
   input << "\tcourt_data = { employer = 5 }\n";
   input << "\tfamily_data = { primary_spouse = 2 spouse = 2 child =  { 9 } concubine = 3 }\n";
   input << "playable_data = { knights = { 9 } }\n";
   input << "alive_data = {obedience_target = 4}\n";
   input << "}\n";
   input << "2 = {\n";
   input << "\tfirst_name = Bob\n";
   input << "\tcourt_data = { employer = 9 }\n";
   input << "\tfamily_data = { primary_spouse = 1 }\n";
   input << "}\n";
   input << "3 = {\n";
   input << "\tfirst_name = Grzegorz\n";
   input << "}\n";
   input << "4 = {\n";
   input << "\tfirst_name = Janusz\n";
   input << "}\n";
   input << "5 = {\n";
   input << "\tfirst_name = Grzmichuj\n";
   input << "}\n";
   input << "9 = {\n";
   input << "\tfirst_name = Carol\n";
   input << "}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input);
   characters.LinkCharacters();

   const auto& character1 = characters.GetAllCharacters().find(1);
   const auto& character2 = characters.GetAllCharacters().find(2);

   ASSERT_EQ("Grzmichuj",
       character1  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetEmployer()
           ->GetPointer()
           .lock()
           ->GetName());
   ASSERT_EQ("Carol",
       character2  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetEmployer()
           ->GetPointer()
           .lock()
           ->GetName());
   ASSERT_EQ("Bob",
       character1->second->GetSpouse()->GetPointer().lock()->GetName());  // NOLINT(bugprone-unchecked-optional-access)
   ASSERT_EQ("Alice",
       character2->second->GetSpouse()->GetPointer().lock()->GetName());  // NOLINT(bugprone-unchecked-optional-access)
   ASSERT_EQ("Janusz",
       character1  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetSuzerain()
           ->GetPointer()
           .lock()
           ->GetName());
   ASSERT_EQ("Grzegorz", character1->second->GetConcubines().at(0).GetPointer().lock()->GetName());
   ASSERT_EQ("Carol", character1->second->GetChildren().at(0).GetPointer().lock()->GetName());
   ASSERT_EQ("Carol", character1->second->GetKnights().at(0).GetPointer().lock()->GetName());
}

TEST(CK3WorldCharactersTests, CharactersRemovedWhenMissingDuringLinking)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "1 = {\n";
   input << "\tfirst_name = Alice\n";
   input << "\tcourt_data = { employer = 5 }\n";
   input << "\tfamily_data = { primary_spouse = 2 spouse = 2 child =  { 9 } concubine = 3 }\n";
   input << "playable_data = { knights = { 9 } }\n";
   input << "alive_data = {obedience_target = 4}\n";
   input << "}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input);
   characters.LinkCharacters();

   const auto& character1 = characters.GetAllCharacters().find(1);

   ASSERT_FALSE(character1->second->GetEmployer().has_value());
   ASSERT_FALSE(character1->second->GetSpouse().has_value());
   ASSERT_FALSE(character1->second->GetSuzerain().has_value());
   ASSERT_EQ(0, character1->second->GetConcubines().size());
   ASSERT_EQ(0, character1->second->GetChildren().size());
   ASSERT_EQ(0, character1->second->GetKnights().size());
}


}  // namespace ck3