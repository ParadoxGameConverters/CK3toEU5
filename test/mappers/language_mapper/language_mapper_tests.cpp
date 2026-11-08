#include <sstream>

#include "gtest/gtest.h"
#include "src/mappers/language_mapper/language_mapper.hpp"

namespace mappers
{

TEST(MappersLanguageMapperTests, MappingsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const LanguageMapper mapper(input);

   ASSERT_TRUE(mapper.GetMappings().empty());
   ASSERT_FALSE(mapper.GetEU5Language("language_arabic").has_value());
}

TEST(MappersLanguageMapperTests, LanguageMapsToLanguage)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = arabic_language ck3 = language_arabic }\n";
   input << "link = { eu5 = greek_language ck3 = language_greek }\n";
   const LanguageMapper mapper(input);

   ASSERT_EQ("arabic_language", mapper.GetEU5Language("language_arabic"));
   ASSERT_EQ("greek_language", mapper.GetEU5Language("language_greek"));
}

TEST(MappersLanguageMapperTests, SeveralLanguagesShareOne)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = serbo_croatian_language ck3 = language_south_slavic ck3 = language_slavonic }\n";
   const LanguageMapper mapper(input);

   ASSERT_EQ(2, mapper.GetMappings().size());
   ASSERT_EQ("serbo_croatian_language", mapper.GetEU5Language("language_slavonic"));
}

TEST(MappersLanguageMapperTests, NameListResolvesToTheSameLanguage)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = arabic_language ck3 = language_arabic name_list = name_list_levantine }\n";
   const LanguageMapper mapper(input);

   ASSERT_EQ("arabic_language", mapper.GetEU5LanguageForNameList("name_list_levantine"));
   // Name lists are kept apart from languages.
   ASSERT_FALSE(mapper.GetEU5Language("name_list_levantine").has_value());
}

TEST(MappersLanguageMapperTests, UnmappedLanguageReturnsNothing)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "link = { eu5 = ??? ck3 = language_mystery }\n";
   input << "link = { eu5 = greek_language ck3 = language_greek }\n";
   const LanguageMapper mapper(input);

   ASSERT_FALSE(mapper.GetEU5Language("language_mystery").has_value());
   ASSERT_EQ("greek_language", mapper.GetEU5Language("language_greek"));
}

}  // namespace mappers
