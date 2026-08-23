#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/geography/province_holdings.hpp"

TEST(CK3WorldProvinceHoldingsTests, ProvinceHoldingsDefaultToEmpty)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::ProvinceHoldings baronies(input);

   ASSERT_TRUE(baronies.GetProvinceHoldings().empty());
}

TEST(CK3WorldProvinceHoldingsTests, ProvinceHoldingsCanBeLoadedIfPresent)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "11={}\n";
   input << "13={holding={}}\n";
   input << "15={holding={type=\"castle_holding\"\nbuildings={ {type=\"hunting_grounds_01\"} {} {} "
            "{type=\"hill_farms_02\"} {} {}}}\n";

   const ck3::ProvinceHoldings baronies(input);
   const auto& barony1 = baronies.GetProvinceHoldings().find(11);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& barony2 = baronies.GetProvinceHoldings().find(13);  // NOLINT(readability-magic-numbers) : "magic number"
   const auto& barony3 = baronies.GetProvinceHoldings().find(15);  // NOLINT(readability-magic-numbers) : "magic number"

   ASSERT_TRUE(barony1->second->GetHoldingType().empty());
   ASSERT_TRUE(barony2->second->GetHoldingType().empty());
   ASSERT_EQ("castle_holding", barony3->second->GetHoldingType());
   ASSERT_EQ(2, barony3->second->GetBuildings().size());
}
