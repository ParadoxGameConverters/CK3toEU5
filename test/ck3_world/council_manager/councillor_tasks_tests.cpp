#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/characters/characters.hpp"
#include "src/ck3_world/council_manager/councillor_tasks.hpp"

namespace ck3
{

TEST(CK3WorldCouncillorTaskTests, CouncillorTasksDefaultsToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const CouncillorTasks councillor_tasks(input);

   ASSERT_EQ(councillor_tasks.GetCouncillorTasks().size(), 0);
}

TEST(CK3WorldCouncillorTaskTests, CouncillorTasksParsesData)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "active={\n";
   input << "1={type=task_foreign_affairs owner=12346 court_owner = 12725}\n";
   input << "2={type=task_foreign_affairs owner=65 court_owner = 78}\n";
   input << "}";
   const CouncillorTasks councillor_tasks(input);

   ASSERT_EQ(councillor_tasks.GetCouncillorTasks().size(), 2);
   ASSERT_EQ(
       councillor_tasks.GetCouncillorTasks().at(1)->GetHolder()->GetID(),  // NOLINT(bugprone-unchecked-optional-access)
       12346);                                                             // NOLINT(readability-magic-numbers)
   ASSERT_EQ(councillor_tasks.GetCouncillorTasks().at(1)->GetCourtOwner().GetID(),
       12725);  // NOLINT(readability-magic-numbers)
   ASSERT_EQ(
       councillor_tasks.GetCouncillorTasks().at(2)->GetHolder()->GetID(),  // NOLINT(bugprone-unchecked-optional-access)
       65);                                                                // NOLINT(readability-magic-numbers)
   ASSERT_EQ(councillor_tasks.GetCouncillorTasks().at(2)->GetCourtOwner().GetID(),
       78);  // NOLINT(readability-magic-numbers)
}

TEST(CK3WorldCouncillorTaskTests, CouncillorTasksSkipsEmptyTasks)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "active={\n";
   input << "1={type=task_foreign_affairs owner=12346 court_owner = 12725}\n";
   input << "2={type=task_foreign_affairs owner=65 court_owner = 78}\n";
   input << "4=none";
   input << "}";
   const CouncillorTasks councillor_tasks(input);

   ASSERT_EQ(councillor_tasks.GetCouncillorTasks().size(), 2);
}

TEST(CK3WorldCouncillorTaskTests, CouncillorTasksLinksCharacters)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "active={\n";
   input << "1={type=task_foreign_affairs owner=1 court_owner = 2}\n";
   input << "}";
   CouncillorTasks councillor_tasks(input);

   std::stringstream input2;
   input2 << "1={first_name=grzegorz}\n";
   input2 << "2={first_name=brzeczeszczykiewicz}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   councillor_tasks.LinkCharacters(characters);

   ASSERT_TRUE(councillor_tasks.GetCouncillorTasks().at(1)->GetHolder().has_value());
   ASSERT_EQ(councillor_tasks.GetCouncillorTasks().at(1)->GetCourtOwner().GetPointer().lock()->GetName(),
       "brzeczeszczykiewicz");
   ASSERT_EQ(councillor_tasks  // NOLINT(bugprone-unchecked-optional-access)
                 .GetCouncillorTasks()
                 .at(1)
                 ->GetHolder()
                 ->GetPointer()
                 .lock()
                 ->GetName(),
       "grzegorz");
}

}  // namespace ck3