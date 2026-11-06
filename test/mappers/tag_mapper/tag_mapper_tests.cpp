#include <sstream>

#include "gtest/gtest.h"
#include "src/mappers/tag_mapper/tag_mapper.hpp"

namespace mappers
{

TEST(MappersTagMapperTests, MappingsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const TagMapper mapper(input);

   ASSERT_TRUE(mapper.GetTitleMappings().empty());
   ASSERT_TRUE(mapper.GetCapitalMappings().empty());
   ASSERT_FALSE(mapper.GetEU5Tag("k_denmark").has_value());
}

TEST(MappersTagMapperTests, TitleMapsToTag)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { ck3 = k_denmark eu5 = DAN }\n";
   input << "link = { ck3 = d_danes eu5 = DAN }\n";
   const TagMapper mapper(input);

   ASSERT_EQ("DAN", mapper.GetEU5Tag("k_denmark"));
   ASSERT_EQ("DAN", mapper.GetEU5Tag("d_danes"));
}

TEST(MappersTagMapperTests, CapitalMapsToTag)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { capitals = { moscow } eu5 = MOS }\n";
   const TagMapper mapper(input);

   ASSERT_EQ("MOS", mapper.GetEU5Tag("d_whatever", "moscow"));
}

TEST(MappersTagMapperTests, CapitalTakesPrecedenceOverTitle)  // NOLINT : clang-tidy doens't like gtest
{
   // Per the file header: e_mongols with a capital in keflavik should become VST, not the Mongol tag.
   std::stringstream input;
   input << "link = { ck3 = e_mongols eu5 = MGE }\n";
   input << "link = { capitals = { keflavik reikjavik } eu5 = VST }\n";
   const TagMapper mapper(input);

   ASSERT_EQ("VST", mapper.GetEU5Tag("e_mongols", "keflavik"));
   ASSERT_EQ("MGE", mapper.GetEU5Tag("e_mongols", "karakorum"));
   ASSERT_EQ("MGE", mapper.GetEU5Tag("e_mongols"));
}

TEST(MappersTagMapperTests, SeveralCapitalsShareOneTag)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { capitals = { keflavik reikjavik } eu5 = VST }\n";
   const TagMapper mapper(input);

   ASSERT_EQ(2, mapper.GetCapitalMappings().size());
   ASSERT_EQ("VST", mapper.GetEU5Tag("c_anything", "reikjavik"));
}

TEST(MappersTagMapperTests, UnresolvedLinksAreSkipped)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { ck3 = k_mystery eu5 = ??? }\n";
   input << "link = { ck3 = k_denmark eu5 = DAN }\n";
   const TagMapper mapper(input);

   ASSERT_FALSE(mapper.GetEU5Tag("k_mystery").has_value());
   ASSERT_EQ("DAN", mapper.GetEU5Tag("k_denmark"));
}

}  // namespace mappers
