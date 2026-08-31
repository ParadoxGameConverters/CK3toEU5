#include <sstream>
#include <stdexcept>

#include "gtest/gtest.h"
#include "src/ck3_world/characters/characters.hpp"
#include "src/ck3_world/council_manager/councillor_task.hpp"

namespace ck3
{
namespace
{
const int kTaskId = 15;

TEST(CK3WorldCouncillorTaskTests, CouncillorTaskDefaultsToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const CouncillorTask councillor_task(input, kTaskId);

   ASSERT_FALSE(councillor_task.GetHolder().has_value());
}

TEST(CK3WorldCouncillorTaskTests, CouncillorTaskParsesData)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "type=task_foreign_affairs owner=12346 court_owner = 12725\n";
   const CouncillorTask councillor_task(input, kTaskId);

   ASSERT_TRUE(councillor_task.GetHolder().has_value());
   ASSERT_EQ(councillor_task.GetCourtOwner().GetID(), 12725);  // NOLINT(readability-magic-numbers)
   ASSERT_EQ(councillor_task.GetHolder()->GetID(),             // NOLINT(bugprone-unchecked-optional-access)
       12346);                                                 // NOLINT(readability-magic-numbers)
   ASSERT_EQ(councillor_task.GetID(), kTaskId);
   ASSERT_EQ(councillor_task.GetType(), "task_foreign_affairs");
}

TEST(CK3WorldCouncillorTaskTests, CouncillorTaskLinksCharacters)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "type=task_foreign_affairs owner=1 court_owner = 2\n";
   CouncillorTask councillor_task(input, kTaskId);

   std::stringstream input2;
   input2 << "1={first_name=grzegorz}\n";
   input2 << "2={first_name=brzeczeszczykiewicz}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   councillor_task.LinkCharacters(characters.GetAliveCharacters());

   ASSERT_TRUE(councillor_task.GetHolder().has_value());
   ASSERT_EQ(councillor_task.GetCourtOwner().GetPointer().lock()->GetName(), "brzeczeszczykiewicz");
   ASSERT_EQ(councillor_task.GetHolder()->GetPointer().lock()->GetName(),  // NOLINT(bugprone-unchecked-optional-access)
       "grzegorz");
}

TEST(CK3WorldCouncillorTaskTests,  // NOLINT : clang-tidy doens't like gtest
    CouncillorTaskLinkingThrowsErrorWhenCourtOwnerMissing)
{
   std::stringstream input;
   input << "type=task_foreign_affairs owner=1 court_owner = 2\n";
   CouncillorTask councillor_task(input, kTaskId);

   std::stringstream input2;
   input2 << "1={first_name=grzegorz}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   ASSERT_THROW(  // NOLINT : clang-tidy doens't like gtest
       councillor_task.LinkCharacters(characters.GetAliveCharacters()),
       std::runtime_error);
}

TEST(CK3WorldCouncillorTaskTests,  // NOLINT : clang-tidy doens't like gtest
    CouncillorTaskLinkingThrowsErrorWhenHolderMissing)
{
   std::stringstream input;
   input << "type=task_foreign_affairs owner=1 court_owner = 2\n";
   CouncillorTask councillor_task(input, kTaskId);

   std::stringstream input2;
   input2 << "2={first_name=grzegorz}\n";
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   ASSERT_THROW(  // NOLINT : clang-tidy doens't like gtest
       councillor_task.LinkCharacters(characters.GetAliveCharacters()),
       std::runtime_error);
}

TEST(CK3WorldCouncillorTaskTests, CouncillorTaskLinkingSkipsWhenNoHolder)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "type=task_foreign_affairs court_owner = 2\n";
   CouncillorTask councillor_task(input, kTaskId);

   std::stringstream input2;
   ck3::Characters characters;
   characters.ParseCharacters(input2);

   councillor_task.LinkCharacters(characters.GetAliveCharacters());
}
}  // namespace

}  // namespace ck3