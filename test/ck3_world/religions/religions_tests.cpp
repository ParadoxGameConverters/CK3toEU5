#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/religions/religions.hpp"

TEST(CK3WorldReligionsTests, ReligionsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Religions religions(input);

   ASSERT_TRUE(religions.GetReligions().empty());
}

TEST(CK3WorldReligionsTests, BundledReligionsCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "religions={\n";
   input << "\t13={tag=\"bon_religion\"}\n";
   input << "\t15={tag=\"buddhism_religion\"}\n";
   input << "}";

   const ck3::Religions religions(input);
   const auto& religion1 = religions.GetReligions().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& religion2 = religions.GetReligions().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ(2, religions.GetReligions().size());
   ASSERT_EQ("bon_religion", religion1->second->GetTag());
   ASSERT_EQ("buddhism_religion", religion2->second->GetTag());
}

TEST(CK3WorldFaithsTests, FaithsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Religions religions(input);

   ASSERT_TRUE(religions.GetFaiths().empty());
}

TEST(CK3WorldFaithsTests, FaithsCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "faiths={\n";
   input << "13={tag=\"old_bon\"}\n";
   input << "15={tag=\"theravada\"}\n";
   input << "}";

   const ck3::Religions religions(input);
   const auto& faith1 = religions.GetFaiths().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& faith2 = religions.GetFaiths().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ("old_bon", faith1->second->GetTag());
   ASSERT_EQ("theravada", faith2->second->GetTag());
}