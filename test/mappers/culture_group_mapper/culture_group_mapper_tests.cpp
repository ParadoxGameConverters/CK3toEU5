#include <sstream>

#include "gtest/gtest.h"
#include "src/mappers/culture_group_mapper/culture_group_mapper.hpp"

namespace mappers
{

TEST(MappersCultureGroupMapperTests, MappingsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const CultureGroupMapper mapper(input);

   ASSERT_TRUE(mapper.GetHeritageMappings().empty());
   ASSERT_TRUE(mapper.GetEU5CultureGroups("heritage_arabic").empty());
}

TEST(MappersCultureGroupMapperTests, HeritageMapsToOneGroup)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = arabic_group heritage = heritage_arabic }\n";
   const CultureGroupMapper mapper(input);

   ASSERT_EQ(1, mapper.GetEU5CultureGroups("heritage_arabic").size());
   ASSERT_EQ("arabic_group", mapper.GetEU5CultureGroups("heritage_arabic").front());
}

TEST(MappersCultureGroupMapperTests, HeritageMapsToSeveralGroups)  // NOLINT : clang-tidy doens't like gtest
{
   // This file is reversed: several eu5 groups per single ck3 heritage.
   std::stringstream input;
   input << "link = { eu5 = amazigh_group eu5 = maghrebi_group heritage = heritage_berber }\n";
   const CultureGroupMapper mapper(input);

   const auto& groups = mapper.GetEU5CultureGroups("heritage_berber");
   ASSERT_EQ(2, groups.size());
   ASSERT_EQ("amazigh_group", groups[0]);
   ASSERT_EQ("maghrebi_group", groups[1]);
}

TEST(MappersCultureGroupMapperTests, LanguageMapsToGroups)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = celtic_group language = language_gaelic }\n";
   const CultureGroupMapper mapper(input);

   ASSERT_EQ(1, mapper.GetEU5CultureGroupsForLanguage("language_gaelic").size());
   ASSERT_TRUE(mapper.GetEU5CultureGroups("language_gaelic").empty());
}

TEST(MappersCultureGroupMapperTests, HeritageAndLanguageOnOneLink)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = british_group heritage = heritage_brythonic language = language_brythonic }\n";
   const CultureGroupMapper mapper(input);

   ASSERT_EQ("british_group", mapper.GetEU5CultureGroups("heritage_brythonic").front());
   ASSERT_EQ("british_group", mapper.GetEU5CultureGroupsForLanguage("language_brythonic").front());
}

TEST(MappersCultureGroupMapperTests, NameListMapsToGroups)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = hibernian_group name_list = name_list_irish }\n";
   const CultureGroupMapper mapper(input);

   ASSERT_EQ("hibernian_group", mapper.GetEU5CultureGroupsForNameList("name_list_irish").front());
   ASSERT_TRUE(mapper.GetEU5CultureGroups("name_list_irish").empty());
}

TEST(MappersCultureGroupMapperTests, PlaceholderGroupsAreSkipped)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = ??? heritage = heritage_akan }\n";
   input << "link = { eu5 = arabic_group heritage = heritage_arabic }\n";
   const CultureGroupMapper mapper(input);

   ASSERT_TRUE(mapper.GetEU5CultureGroups("heritage_akan").empty());
   ASSERT_EQ(1, mapper.GetHeritageMappings().size());
}

}  // namespace mappers
