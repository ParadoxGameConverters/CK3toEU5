#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"
#include "src/ck3_world/characters/character_realm.hpp"
#include "src/ck3_world/council_manager/councillor_tasks.hpp"
#include "src/ck3_world/titles/title.hpp"
#include "src/ck3_world/titles/titles.hpp"

namespace ck3
{

TEST(CK3WorldCharactersRealmTests, CharacterRealmDefaultsToEmpty)  // NOLINT - readability-function-cognitive-complexity
{
   std::stringstream input;
   CharacterRealm character_realm(input);

   ASSERT_EQ(character_realm.GetVassalPower(), std::nullopt);
   ASSERT_EQ(character_realm.GetRealmCapital(), std::nullopt);
   ASSERT_TRUE(character_realm.GetCourtLanguage().empty());
   ASSERT_EQ(character_realm.GetCouncil().size(), 0);
   ASSERT_EQ(character_realm.GetDomain().size(), 0);
   ASSERT_TRUE(character_realm.GetGovernmentType().empty());
   ASSERT_EQ(character_realm.GetLaws().size(), 0);
}

TEST(CK3WorldCharactersRealmTests, CharacterRealmParsed)  // NOLINT - readability-function-cognitive-complexity
{
   std::stringstream input;
   input << "vassal_power_value = 1337\n";
   input << "laws={ japanese_bureaucracy_2 single_heir_succession_law }\n";
   input << "realm_capital = 13414\n";
   input << "domain={ 13176 13177 13297}\n";
   input << "government=japan_feudal_government\n";
   input << "council={ 16795546 16795548 16795549 33556428 33572763 33574740 }\n";
   input << "royal_court={ language=language_japonic }\n";
   CharacterRealm character_realm(input);

   ASSERT_EQ(character_realm.GetVassalPower(), 1337);
   ASSERT_TRUE(character_realm.GetRealmCapital().has_value());
   ASSERT_EQ(character_realm.GetRealmCapital()->GetID(), 13414);  // NOLINT : bugprone-unchecked-optional-access
   ASSERT_EQ(character_realm.GetCourtLanguage(), "language_japonic");
   ASSERT_EQ(character_realm.GetCouncil().size(), 6);
   ASSERT_EQ(character_realm.GetDomain().size(), 3);
   ASSERT_EQ(character_realm.GetGovernmentType(), "japan_feudal_government");
   ASSERT_EQ(character_realm.GetLaws().size(), 2);
}

TEST(CK3WorldCharactersTests, WarningLoggedWhenDifferentOwners)  // NOLINT : clang-tidy doens't like gtest
{
   const std::stringstream log;
   std::streambuf* cout_buffer = std::cout.rdbuf();
   std::cout.rdbuf(log.rdbuf());

   std::stringstream input;
   input << "landed_titles={\n";
   input << "1 = { key=c_county1 }\n";
   input << "2 = { key=c_county2 }\n";
   input << "3 = { key=c_county3 }\n";
   input << "}";
   const ck3::Titles titles(input);

   std::stringstream input3;
   input3 << "active={\n";
   input3 << "101 = { type=task_foreign_affairs   owner=123   court_owner = 115 }\n";
   input3 << "103 = { type=task_foreign_affairs   owner=124   court_owner = 115 }\n";
   input3 << "}\n";
   const ck3::CouncillorTasks councillor_tasks(input3);

   std::stringstream input2;
   input2 << "\t\trealm_capital = 2\n";
   input2 << "\t\tdomain = { 1 2 }";
   input2 << "\t\tcouncil={ 101 103 }\n";
   ck3::CharacterRealm character_realm(input2);

   std::map<long long, std::shared_ptr<Title>> id_title_map;
   for (const auto& title: titles.GetTitles())
   {
      id_title_map.insert(std::pair(title.second->GetID(), title.second));
   }

   character_realm.Link(id_title_map,
       councillor_tasks.GetCouncillorTasks(),
       100);  // NOLINT(readability-magic-numbers) : "magic number"

   EXPECT_THAT(log.str(),
       testing::HasSubstr(R"(Task 101 claims different court_owner 115 than the councillor's realm owner 100)"));

   std::cout.rdbuf(cout_buffer);
}

TEST(CK3WorldCharactersTests, WarningLoggedWhenNonexistantCouncilTask)  // NOLINT : clang-tidy doens't like gtest
{
   const std::stringstream log;
   std::streambuf* cout_buffer = std::cout.rdbuf();
   std::cout.rdbuf(log.rdbuf());

   std::stringstream input;
   input << "landed_titles={\n";
   input << "1 = { key=c_county1 }\n";
   input << "2 = { key=c_county2 }\n";
   input << "3 = { key=c_county3 }\n";
   input << "}";
   const ck3::Titles titles(input);

   std::stringstream input3;
   input3 << "active={\n";
   input3 << "101 = { type=task_foreign_affairs   owner=123   court_owner = 100 }\n";
   input3 << "103 = { type=task_foreign_affairs   owner=124   court_owner = 100 }\n";
   input3 << "}\n";
   const ck3::CouncillorTasks councillor_tasks(input3);

   std::stringstream input2;
   input2 << "\t\trealm_capital = 2\n";
   input2 << "\t\tdomain = { 1 2 }";
   input2 << "\t\tcouncil={ 101 103 106 }\n";  // councillor task 106 doesn't exist
   ck3::CharacterRealm character_realm(input2);

   std::map<long long, std::shared_ptr<Title>> id_title_map;
   for (const auto& title: titles.GetTitles())
   {
      id_title_map.insert(std::pair(title.second->GetID(), title.second));
   }

   character_realm.Link(id_title_map,
       councillor_tasks.GetCouncillorTasks(),
       100);  // NOLINT(readability-magic-numbers) : "magic number"

   EXPECT_THAT(log.str(), testing::HasSubstr(R"(Missing councillor task 106 when linking realm of 100)"));

   std::cout.rdbuf(cout_buffer);
}

TEST(CK3WorldCharactersTests, ThrowsWhenNonexistentTitle)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "landed_titles={\n";
   input << "1 = { key=c_county1 }\n";
   input << "2 = { key=c_county2 }\n";
   input << "3 = { key=c_county3 }\n";
   input << "}";
   const ck3::Titles titles(input);

   std::stringstream input3;
   input3 << "active={\n";
   input3 << "101 = { type=task_foreign_affairs   owner=123   court_owner = 100 }\n";
   input3 << "103 = { type=task_foreign_affairs   owner=124   court_owner = 100 }\n";
   input3 << "}\n";
   const ck3::CouncillorTasks councillor_tasks(input3);

   std::stringstream input2;
   input2 << "\t\trealm_capital = 2\n";
   input2 << "\t\tdomain = { 6 2 }";  // title 6 doesn't exist
   input2 << "\t\tcouncil={ 101 103 }\n";
   ck3::CharacterRealm character_realm(input2);

   std::map<long long, std::shared_ptr<Title>> id_title_map;
   for (const auto& title: titles.GetTitles())
   {
      id_title_map.insert(std::pair(title.second->GetID(), title.second));
   }

   ASSERT_THROW(character_realm.Link(id_title_map,  // NOLINT : clang-tidy doens't like gtest
                    councillor_tasks.GetCouncillorTasks(),
                    100),  // NOLINT(readability-magic-numbers) : "magic number"
       std::runtime_error);
}

TEST(CK3WorldCharactersTests, ThrowsWhenNonexistentCapital)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "landed_titles={\n";
   input << "1 = { key=c_county1 }\n";
   input << "2 = { key=c_county2 }\n";
   input << "3 = { key=c_county3 }\n";
   input << "}";
   const ck3::Titles titles(input);

   std::stringstream input3;
   input3 << "active={\n";
   input3 << "101 = { type=task_foreign_affairs   owner=123   court_owner = 100 }\n";
   input3 << "103 = { type=task_foreign_affairs   owner=124   court_owner = 100 }\n";
   input3 << "}\n";
   const ck3::CouncillorTasks councillor_tasks(input3);

   std::stringstream input2;
   input2 << "\t\trealm_capital = 6\n";  // title 6 doesn't exist
   input2 << "\t\tdomain = { 1 2 }";
   input2 << "\t\tcouncil={ 101 103 }\n";
   ck3::CharacterRealm character_realm(input2);

   std::map<long long, std::shared_ptr<Title>> id_title_map;
   for (const auto& title: titles.GetTitles())
   {
      id_title_map.insert(std::pair(title.second->GetID(), title.second));
   }

   ASSERT_THROW(character_realm.Link(id_title_map,  // NOLINT : clang-tidy doens't like gtest
                    councillor_tasks.GetCouncillorTasks(),
                    100),  // NOLINT(readability-magic-numbers) : "magic number"
       std::runtime_error);
}

}  // namespace ck3