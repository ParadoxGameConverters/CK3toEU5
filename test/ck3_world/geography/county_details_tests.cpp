#include <sstream>
#include <stdexcept>

#include "gtest/gtest.h"
#include "src/ck3_world/cultures/cultures.hpp"
#include "src/ck3_world/geography/county_detail.hpp"
#include "src/ck3_world/geography/county_details.hpp"
#include "src/ck3_world/religions/religions.hpp"

TEST(CK3WorldCountyDetailsTests, DetailsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::CountyDetails details(input);

   ASSERT_TRUE(details.GetCountyDetails().empty());
}

TEST(CK3WorldCountyDetailsTests, DetailsCanBeLoadedIfPresent)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "counties = {\n";
   input << "c_county1 = {}\n";
   input << "c_county2 = { development = 8 culture = 3 faith = 9}\n";
   input << "c_county3 = { faith = 4 }\n";
   input << "}";

   const ck3::CountyDetails details(input);
   const auto& county1 = details.GetCountyDetails().find("c_county1");
   const auto& county2 = details.GetCountyDetails().find("c_county2");
   const auto& county3 = details.GetCountyDetails().find("c_county3");

   ASSERT_EQ(3, details.GetCountyDetails().size());
   ASSERT_EQ(0, county1->second->GetDevelopment());
   ASSERT_EQ(8, county2->second->GetDevelopment());
   ASSERT_EQ(9, county2->second->GetFaith().GetID());
   ASSERT_EQ(3, county2->second->GetCulture().GetID());
   ASSERT_EQ(4, county3->second->GetFaith().GetID());
}

TEST(CK3WorldCountyDetailsTests, FaithsCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "counties = {\n";
   input << "c_county2 = { development = 8 culture = 3 faith = 9}\n";
   input << "c_county3 = { culture = 3 faith = 4 }\n";
   input << "}";
   ck3::CountyDetails details(input);

   std::stringstream input2;
   input2 << "faiths={\n";
   input2 << "9={tag=\"old_bon\"}\n";
   input2 << "4={tag=\"theravada\"}\n";
   input2 << "}";
   const ck3::Religions religions(input2);

   details.LinkReligions(religions);
   const auto& county2 = details.GetCountyDetails().find("c_county2");
   const auto& county3 = details.GetCountyDetails().find("c_county3");

   ASSERT_EQ("old_bon", county2->second->GetFaith().GetPointer().lock()->GetTag());
   ASSERT_EQ("theravada", county3->second->GetFaith().GetPointer().lock()->GetTag());
}

TEST(CK3WorldCountyDetailsTests, LinkingMissingFaithThrowsError)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "counties = {\n";
   input << "c_county2 = { development = 8 culture = 3 faith = 9}\n";
   input << "c_county3 = { culture = 3 faith = 6 }\n";  // 6 is missing
   input << "}";
   ck3::CountyDetails details(input);

   std::stringstream input2;
   input2 << "faiths={\n";
   input2 << "9={tag=\"old_bon\"}\n";
   input2 << "4={tag=\"theravada\"}\n";
   input2 << "}";
   const ck3::Religions religions(input2);

   ASSERT_THROW(details.LinkReligions(religions),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}

TEST(CK3WorldCountyDetailsTests, CulturesCanBeLinked)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "counties = {\n";
   input << "c_county2 = { development = 8 culture = 13 faith = 9}\n";
   input << "c_county3 = { culture = 15 faith = 9 }\n";
   input << "}";
   ck3::CountyDetails details(input);

   std::stringstream input2;
   input2 << "cultures={\n";
   input2 << "\t13={culture_template=\"akan\"}\n";
   input2 << "\t15={culture_template=\"kru\"}\n";
   input2 << "}\n";
   const ck3::Cultures cultures(input2);

   details.LinkCultures(cultures);
   const auto& county2 = details.GetCountyDetails().find("c_county2");
   const auto& county3 = details.GetCountyDetails().find("c_county3");

   ASSERT_EQ("akan", county2->second->GetCulture().GetPointer().lock()->GetTemplate());
   ASSERT_EQ("kru", county3->second->GetCulture().GetPointer().lock()->GetTemplate());
}

TEST(CK3WorldCountyDetailsTests, LinkingMissingCultureThrowsError)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "counties = {\n";
   input << "c_county2 = { development = 8 culture = 3 faith = 9}\n";
   input << "c_county3 = { culture = 6 faith = 9 }\n";  // 6 is missing
   input << "}";
   ck3::CountyDetails details(input);

   std::stringstream input2;
   input2 << "cultures={\n";
   input2 << "\t13={culture_template=\"akan\"}\n";
   input2 << "\t15={culture_template=\"kru\"}\n";
   input2 << "}\n";
   const ck3::Cultures cultures(input2);

   ASSERT_THROW(details.LinkCultures(cultures),  // NOLINT : clang-tidy doens't like gtest
       std::runtime_error);
}