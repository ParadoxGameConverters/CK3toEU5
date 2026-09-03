#include <sstream>
#include <stdexcept>

#include "gtest/gtest.h"
#include "src/ck3_world/characters/characters.hpp"
#include "src/ck3_world/religions/religions.hpp"

namespace ck3
{

TEST(CK3WorldReligionsTests, ReligionsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const Religions religions(input);

   ASSERT_TRUE(religions.GetReligions().empty());
}

TEST(CK3WorldReligionsTests, BundledReligionsCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "religions={\n";
   input << "\t13={tag=\"bon_religion\"}\n";
   input << "\t15={tag=\"buddhism_religion\"}\n";
   input << "}";

   const Religions religions(input);
   const auto& religion1 = religions.GetReligions().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& religion2 = religions.GetReligions().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ(2, religions.GetReligions().size());
   ASSERT_EQ("bon_religion", religion1->second->GetTag());
   ASSERT_EQ("buddhism_religion", religion2->second->GetTag());
}

TEST(CK3WorldFaithsTests, FaithsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const Religions religions(input);

   ASSERT_TRUE(religions.GetFaiths().empty());
}

TEST(CK3WorldFaithsTests, FaithsCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "faiths={\n";
   input << "13={tag=\"old_bon\"}\n";
   input << "15={tag=\"theravada\"}\n";
   input << "}";

   const Religions religions(input);
   const auto& faith1 = religions.GetFaiths().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& faith2 = religions.GetFaiths().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ("old_bon", faith1->second->GetTag());
   ASSERT_EQ("theravada", faith2->second->GetTag());
}

TEST(CK3WorldFaithsTests, FaithCharactersCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input2;
   input2 << "11={}\n";
   input2 << "17={first_name=\"bob\"}\n";
   input2 << "18={first_name=\"alice\"}\n";

   Characters characters;
   characters.ParseCharacters(input2);

   std::stringstream input;
   input << "faiths={\n";
   input << "13={tag=\"old_bon\" religious_head=17}\n";
   input << "15={tag=\"theravada\" religious_head=18}\n";
   input << "}";
   Religions religions(input);

   religions.LinkCharacters(characters);

   const auto& faith1 = religions.GetFaiths().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& faith2 = religions.GetFaiths().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ("bob",
       faith1  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetReligionHead()
           ->GetPointer()
           .lock()
           ->GetName());
   ASSERT_EQ("alice",
       faith2  // NOLINT(bugprone-unchecked-optional-access)
           ->second->GetReligionHead()
           ->GetPointer()
           .lock()
           ->GetName());
}

TEST(CK3WorldFaithsTests, LinkingMissingReligiousHeadThrowsError)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input2;
   input2 << "11={}\n";
   input2 << "17={first_name=\"bob\"}\n";
   input2 << "18={first_name=\"alice\"}\n";

   Characters characters;
   characters.ParseCharacters(input2);

   std::stringstream input;
   input << "faiths={\n";
   input << "13={tag=\"old_bon\" religious_head=17}\n";
   input << "15={tag=\"theravada\" religious_head=6}\n";  // 6 is missing
   input << "}";
   Religions religions(input);

   ASSERT_THROW(religions.LinkCharacters(characters), std::runtime_error);  // NOLINT : clang-tidy doens't like gtest
}

TEST(CK3WorldFaithsTests, ReligionsCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "religions={\n";
   input << "\t11={tag=\"bon_religion\" faiths={ 13 15 }}\n";
   input << "}";
   input << "faiths={\n";
   input << "13={tag=\"old_bon\" religion=11}\n";
   input << "15={tag=\"theravada\" religion=11}\n";
   input << "}";
   Religions religions(input);

   religions.LinkReligions();

   const auto& faith1 = religions.GetFaiths().find(13);        // NOLINT(readability-magic-numbers) : "magic number"
   const auto& faith2 = religions.GetFaiths().find(15);        // NOLINT(readability-magic-numbers) : "magic number"
   const auto& religion1 = religions.GetReligions().find(11);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_EQ("bon_religion", faith1->second->GetReligion().GetPointer().lock()->GetTag());
   ASSERT_EQ("bon_religion", faith2->second->GetReligion().GetPointer().lock()->GetTag());

   ASSERT_EQ("old_bon", religion1->second->GetFaiths()[0].GetPointer().lock()->GetTag());
   ASSERT_EQ("theravada", religion1->second->GetFaiths()[1].GetPointer().lock()->GetTag());
}

TEST(CK3WorldFaithsTests, LinkingMissingReligionThrowsError)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "religions={\n";
   input << "\t11={tag=\"bon_religion\" faiths={ 13 15 }}\n";
   input << "}";
   input << "faiths={\n";
   input << "13={tag=\"old_bon\" religion=6}\n";  // 6 is missing
   input << "15={tag=\"theravada\" religion=11}\n";
   input << "}";
   Religions religions(input);

   ASSERT_THROW(religions.LinkReligions(), std::runtime_error);  // NOLINT : clang-tidy doens't like gtest
}

TEST(CK3WorldFaithsTests, LinkingMissingFaithThrowsError)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "religions={\n";
   input << "\t11={tag=\"bon_religion\" faiths={ 13 15 6 }}\n";  // 6 is missing
   input << "}";
   input << "faiths={\n";
   input << "13={tag=\"old_bon\" religion=11}\n";
   input << "15={tag=\"theravada\" religion=11}\n";
   input << "}";
   Religions religions(input);

   ASSERT_THROW(religions.LinkReligions(), std::runtime_error);  // NOLINT : clang-tidy doens't like gtest
}

}  // namespace ck3