#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/titles/title.hpp"
#include "src/ck3_world/titles/titles.hpp"

TEST(CK3WorldTitlesTests, TitlesDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::Titles titles(input);

   ASSERT_TRUE(titles.GetTitles().empty());
}

TEST(CK3WorldTitlesTests, TitlesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "landed_titles={\n";
   input << "123={key=c_roma holder=21}\n";
   input << "12345={key=c_roma2 holder=21}\n";
   input << "}";

   const ck3::Titles titles(input);
   const auto& roma = titles.GetTitles().find("c_roma");
   const auto& roma2 = titles.GetTitles().find("c_roma2");

   ASSERT_EQ("c_roma", roma->second->GetKey());
   ASSERT_EQ(123, roma->second->GetID());
   ASSERT_EQ("c_roma2", roma2->second->GetKey());
   ASSERT_EQ(12345, roma2->second->GetID());
}

TEST(CK3WorldTitlesTests, TitleLevelAssigned)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "landed_titles={\n";
   input << "123={key=b_roma holder=21}\n";
   input << "12345={key=c_roma holder=21}\n";
   input << "56={key=d_roma holder=21}\n";
   input << "67={key=k_roma holder=21}\n";
   input << "78={key=e_roma holder=21}\n";
   input << "89={key=h_roma holder=21}\n";
   input << "}";

   const ck3::Titles titles(input);
   const auto& c_roma = titles.GetTitles().find("c_roma");
   const auto& d_roma = titles.GetTitles().find("d_roma");
   const auto& k_roma = titles.GetTitles().find("k_roma");
   const auto& e_roma = titles.GetTitles().find("e_roma");
   const auto& h_roma = titles.GetTitles().find("h_roma");
   const auto& b_roma = titles.GetTitles().find("b_roma");

   ASSERT_EQ("b_roma", b_roma->second->GetKey());
   ASSERT_EQ(123, b_roma->second->GetID());
   ASSERT_EQ("c_roma", c_roma->second->GetKey());
   ASSERT_EQ(12345, c_roma->second->GetID());
   ASSERT_EQ("d_roma", d_roma->second->GetKey());
   ASSERT_EQ(56, d_roma->second->GetID());
   ASSERT_EQ("k_roma", k_roma->second->GetKey());
   ASSERT_EQ(67, k_roma->second->GetID());
   ASSERT_EQ("e_roma", e_roma->second->GetKey());
   ASSERT_EQ(78, e_roma->second->GetID());
   ASSERT_EQ("h_roma", h_roma->second->GetKey());
   ASSERT_EQ(89, h_roma->second->GetID());
}

TEST(CK3WorldTitlesTests, TitlesDividedIntoLevels)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "landed_titles={\n";
   input << "123={key=b_roma holder=21}\n";
   input << "12345={key=c_roma holder=21}\n";
   input << "56={key=d_roma holder=21}\n";
   input << "67={key=k_roma holder=21}\n";
   input << "78={key=e_roma holder=21}\n";
   input << "89={key=h_roma holder=21}\n";
   input << "}";

   const ck3::Titles titles(input);

   ASSERT_TRUE(titles.GetBaronies().contains("b_roma"));
   ASSERT_TRUE(titles.GetCounties().contains("c_roma"));
   ASSERT_TRUE(titles.GetDuchies().contains("d_roma"));
   ASSERT_TRUE(titles.GetKingdoms().contains("k_roma"));
   ASSERT_TRUE(titles.GetEmpires().contains("e_roma"));
   ASSERT_TRUE(titles.GetHegemonies().contains("h_roma"));
}

TEST(CK3WorldTitlesTests, NamelessTitlesAreIgnored)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "landed_titles={\n";
   input << "123={key=c_roma holder=21}\n";
   input << "1234={}\n";
   input << "12345={key=c_roma2 holder=21}\n";
   input << "}";

   const ck3::Titles titles(input);

   ASSERT_EQ(2, titles.GetTitles().size());
}

TEST(CK3WorldTitlesTests, InsaneTitlesThrowException)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "landed_titles={\n";
   input << "123={key=c_roma}\n";
   input << "1234={}\n";
   input << "1234512356789012345678901234567890={key=c_roma2}\n";
   input << "}";

   EXPECT_ANY_THROW(const ck3::Titles titles(input));  // NOLINT : gtest things (multiple rules)
}

TEST(CK3WorldTitlesTests, JunkTitlesAreIgnored)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "landed_titles={\n";
   input << "123={key=c_roma  holder=21}\n";
   input << "1234=none\n";
   input << "12345={key=c_roma2  holder=21}\n";
   input << "123465={key=c_roma2}\n";
   input << "}";

   const ck3::Titles titles(input);

   ASSERT_EQ(2, titles.GetTitles().size());
}

TEST(CK3WorldTitlesTests, DynamicTitleLevelsAreLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "landed_titles={\n";
   input << "123={key=x_roma  holder=21}\n";
   input << "12345={key=x_gg  holder=21}\n";
   input << "}";
   input << "dynamic_templates={\n";
   input << "{key=x_roma tier=duchy dyn=yes}\n";
   input << "{key=x_mc_3 tier=barony dyn=yes}\n";
   input << "{key=x_gg tier=barony dyn=yes}\n";
   input << "{key=x_ff tier=duchy dyn=yes}\n";
   input << "}";

   const ck3::Titles titles(input);

   ASSERT_EQ(2, titles.GetTitles().size());
   ASSERT_EQ(1, titles.GetBaronies().size());
   ASSERT_EQ(1, titles.GetDuchies().size());

   ASSERT_EQ(ck3::Level::kBarony, titles.GetTitles().at("x_gg")->GetLevel());
   ASSERT_EQ(ck3::Level::kDuchy, titles.GetTitles().at("x_roma")->GetLevel());
}

TEST(CK3WorldTitlesTests, DynamicTitleLevelsAllWork)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "landed_titles={\n";
   input << "123={key=x_roma_b  holder=21}\n";
   input << "125={key=x_roma_c  holder=21}\n";
   input << "126={key=x_roma_d  holder=21}\n";
   input << "127={key=x_roma_k  holder=21}\n";
   input << "128={key=x_roma_e  holder=21}\n";
   input << "129={key=x_roma_h  holder=21}\n";
   input << "}";
   input << "dynamic_templates={\n";
   input << "{key=x_roma_b tier=barony dyn=yes}\n";
   input << "{key=x_roma_c tier=county dyn=yes}\n";
   input << "{key=x_roma_d tier=duchy dyn=yes}\n";
   input << "{key=x_roma_k tier=kingdom dyn=yes}\n";
   input << "{key=x_roma_e tier=empire dyn=yes}\n";
   input << "{key=x_roma_h tier=hegemony dyn=yes}\n";
   input << "}";

   const ck3::Titles titles(input);

   ASSERT_EQ(6, titles.GetTitles().size());

   ASSERT_EQ(ck3::Level::kBarony, titles.GetTitles().at("x_roma_b")->GetLevel());
   ASSERT_EQ(ck3::Level::kCounty, titles.GetTitles().at("x_roma_c")->GetLevel());
   ASSERT_EQ(ck3::Level::kDuchy, titles.GetTitles().at("x_roma_d")->GetLevel());
   ASSERT_EQ(ck3::Level::kKingdom, titles.GetTitles().at("x_roma_k")->GetLevel());
   ASSERT_EQ(ck3::Level::kEmpire, titles.GetTitles().at("x_roma_e")->GetLevel());
   ASSERT_EQ(ck3::Level::kHegemony, titles.GetTitles().at("x_roma_h")->GetLevel());
}