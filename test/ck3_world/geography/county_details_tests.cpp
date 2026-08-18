#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/geography/county_detail.hpp"
#include "src/ck3_world/geography/county_details.hpp"

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
