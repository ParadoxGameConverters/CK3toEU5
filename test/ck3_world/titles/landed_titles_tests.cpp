#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/titles/landed_titles.hpp"

TEST(CK3WorldLandedTitlesTests, TitlePrimitivesDefaultToBlank)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   ck3::LandedTitles titles;
   titles.LoadTitles(input);

   ASSERT_TRUE(titles.GetLandedTitles().empty());
}

TEST(CK3WorldLandedTitlesTests, TitlePrimitivesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "b_barony = {\n";
   input << "definite_form = yes\n";
   input << "landless = yes\n";
   input << "color = { 23 23 23 }\n";
   input << "capital = c_roma\n";
   input << "province = 345\n";
   input << "ruler_uses_title_name = yes\n";
   input << "can_be_named_after_dynasty = yes\n";
   input << "}";

   ck3::LandedTitles titles;
   titles.LoadTitles(input);
   const auto title = titles.GetLandedTitles().at("b_barony");

   ASSERT_TRUE(title->IsDefiniteForm());
   ASSERT_TRUE(title->IsLandless());
   ASSERT_TRUE(title->DoesRulerUseTitleName());
   ASSERT_TRUE(title->CanBeNamedAfterDynasty());
   ASSERT_EQ(345, title->GetProvince());
   ASSERT_EQ("b_barony", title->GetTitleKey());
}

TEST(CK3WorldLandedTitlesTests, TitlesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "b_barony = { province = 12 }\n";
   input << "c_county = { landless = yes }\n";

   ck3::LandedTitles titles;
   titles.LoadTitles(input);

   const auto& barony = titles.GetLandedTitles().find("b_barony");
   const auto& county = titles.GetLandedTitles().find("c_county");

   ASSERT_EQ(2, titles.GetLandedTitles().size());
   ASSERT_EQ(12, barony->second->GetProvince());
   ASSERT_TRUE(county->second->IsLandless());
}

TEST(CK3WorldLandedTitlesTests, TitlesCanBeLoadedRecursively)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "e_empire1 = { k_kingdom2 = { d_duchy3 = { b_barony4 = { province = 12 } } } }\n";
   input << "c_county5 = { landless = yes }\n";

   ck3::LandedTitles titles;
   titles.LoadTitles(input);

   const auto& barony = titles.GetLandedTitles().find("b_barony4");
   const auto& county = titles.GetLandedTitles().find("c_county5");

   ASSERT_EQ(5, titles.GetLandedTitles().size());
   ASSERT_EQ(12, barony->second->GetProvince());
   ASSERT_TRUE(county->second->IsLandless());
}

TEST(CK3WorldLandedTitlesTests, TitlesCanBeOverriddenByMods)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "e_empire1 = { k_kingdom2 = { d_duchy3 = { b_barony4 = { province = 12 } } } }\n";
   input << "c_county5 = { landless = yes }\n";

   ck3::LandedTitles titles;
   titles.LoadTitles(input);

   std::stringstream input2;
   input2 << "b_barony4 = { province = 15 }\n";
   input2 << "c_county5 = { landless = no }\n";
   titles.LoadTitles(input2);

   const auto& barony = titles.GetLandedTitles().find("b_barony4");
   const auto& county = titles.GetLandedTitles().find("c_county5");

   ASSERT_EQ(5, titles.GetLandedTitles().size());
   ASSERT_EQ(15, barony->second->GetProvince());
   ASSERT_FALSE(county->second->IsLandless());
}

TEST(CK3WorldLandedTitlesTests, TitlesCanBeAddedByMods)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "e_empire1 = { k_kingdom2 = { d_duchy3 = { b_barony4 = { province = 12 } } } }\n";
   input << "c_county5 = { landless = yes }\n";

   ck3::LandedTitles titles;
   titles.LoadTitles(input);

   std::stringstream input2;
   input2 << "c_county5 = { landless = no }\n";  // Overrides existing instance
   input2 << "e_empire6 = { k_kingdom7 = { d_duchy8 = { b_barony9 = { province = 12 } } } }\n";
   input2 << "c_county10 = { landless = yes }\n";
   titles.LoadTitles(input2);

   ASSERT_EQ(10, titles.GetLandedTitles().size());
}
