#include <sstream>

#include "gtest/gtest.h"
#include "src/mappers/religion_mapper/religion_mapper.hpp"

namespace mappers
{

TEST(MappersReligionMapperTests, MappingsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ReligionMapper mapper(input);

   ASSERT_TRUE(mapper.GetMappings().empty());
   ASSERT_FALSE(mapper.GetEU5Religion("catholic").has_value());
}

TEST(MappersReligionMapperTests, FaithMapsToReligion)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = bon ck3 = bon ck3 = old_bon }\n";
   const ReligionMapper mapper(input);

   ASSERT_EQ("bon", mapper.GetEU5Religion("bon"));
   ASSERT_EQ("bon", mapper.GetEU5Religion("old_bon"));
}

TEST(MappersReligionMapperTests, SeveralFaithsShareOneReligion)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = mahayana ck3 = mahayana ck3 = ari ck3 = avatamsaka ck3 = mantrayana }\n";
   const ReligionMapper mapper(input);

   ASSERT_EQ(4, mapper.GetMappings().size());
   ASSERT_EQ("mahayana", mapper.GetEU5Religion("avatamsaka"));
   ASSERT_EQ("mahayana", mapper.GetEU5Religion("mantrayana"));
}

TEST(MappersReligionMapperTests, UnmappedFaithReturnsNothing)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = bon ck3 = bon }\n";
   const ReligionMapper mapper(input);

   ASSERT_FALSE(mapper.GetEU5Religion("zoroastrian").has_value());
}

TEST(MappersReligionMapperTests, LinkWithoutAnEU5ReligionIsSkipped)  // NOLINT : clang-tidy doens't like gtest
{
   // The shipped file carries unresolved entries as ??? placeholders.
   std::stringstream input;
   input << "link = { eu5 = ??? ck3 = mystery }\n";
   input << "link = { ck3 = orphan }\n";
   input << "link = { eu5 = bon ck3 = bon }\n";
   const ReligionMapper mapper(input);

   ASSERT_FALSE(mapper.GetEU5Religion("mystery").has_value());
   ASSERT_FALSE(mapper.GetEU5Religion("orphan").has_value());
   ASSERT_EQ("bon", mapper.GetEU5Religion("bon"));
}

TEST(MappersReligionMapperTests, CommentsAndTrailingTextAreIgnored)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "# a comment\n";
   input << "link = { eu5 = shinto ck3 = shinto ck3 = shugendo } #Shinto and Shugendo\n";
   const ReligionMapper mapper(input);

   ASSERT_EQ(2, mapper.GetMappings().size());
   ASSERT_EQ("shinto", mapper.GetEU5Religion("shugendo"));
}

}  // namespace mappers
