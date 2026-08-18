#include <sstream>

#include "gtest/gtest.h"
#include "src/ck3_world/geography/province_holding.hpp"

TEST(CK3WorldProvinceHoldingTests, LoadValuesDefaultToBlank)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   const ck3::ProvinceHolding holding(input);

   ASSERT_TRUE(holding.GetHoldingType().empty());
   ASSERT_TRUE(holding.GetSpecialBuilding().empty());
   ASSERT_TRUE(holding.GetBuildings().empty());
   ASSERT_NEAR(0, holding.GetIncome(), 0.001);
   ASSERT_NEAR(0, holding.GetBarterGoods(), 0.001);
}

TEST(CK3WorldProvinceHoldingTests, BlankHoldingIsBlank)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "holding = {}";

   const ck3::ProvinceHolding holding(input);

   ASSERT_TRUE(holding.GetHoldingType().empty());
   ASSERT_TRUE(holding.GetSpecialBuilding().empty());
   ASSERT_TRUE(holding.GetBuildings().empty());
   ASSERT_NEAR(0, holding.GetIncome(), 0.001);
   ASSERT_NEAR(0, holding.GetBarterGoods(), 0.001);
}

TEST(CK3World_ProvinceHoldingTests, holdingPrimitivesCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "holding = {\n";
   input << "\ttype = \"city_holding\"\n";
   input << "\tspecial_building_type = \"holy_site_cathedral_01\"\n";
   input << "income = 34.8\n";
   input << "barter_goods = 6.9\n";
   input << "}";

   const ck3::ProvinceHolding holding(input);

   ASSERT_EQ("city_holding", holding.GetHoldingType());
   ASSERT_EQ("holy_site_cathedral_01", holding.GetSpecialBuilding());
   ASSERT_NEAR(34.8, holding.GetIncome(), 0.001);
   ASSERT_NEAR(6.9, holding.GetBarterGoods(), 0.001);
}

TEST(CK3WorldProvinceHoldingTests, HoldingEmptyBuildingBlobsAreIgnored)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "holding = {\n";
   input << "\tbuildings = {\n";
   input << "\t\n{\n}\n{\n}\n{\n}{}{}\n";
   input << "\t}\n";
   input << "}";

   const ck3::ProvinceHolding holding(input);

   ASSERT_TRUE(holding.GetBuildings().empty());
}

TEST(CK3WorldProvinceHoldingTests, HoldingBuildingBlobsCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "holding = {\n";
   input << "\tbuildings = {\n";
   input << "\t\n{\ntype = \"castle_01\"\n}{type = \"hill_farms_02\"}\n";
   input << "\t}\n";
   input << "duchy_capital_building={disabled = yes type = tax_assessor_01}\n";
   input << "}";

   const ck3::ProvinceHolding holding(input);

   ASSERT_EQ(2, holding.GetBuildings().size());
   ASSERT_EQ(holding.GetBuildings()[0].GetType(), "castle");
   ASSERT_EQ(holding.GetBuildings()[0].GetLevel(), 1);
   ASSERT_EQ(holding.GetBuildings()[1].GetType(), "hill_farms");
   ASSERT_EQ(holding.GetBuildings()[1].GetLevel(), 2);
}

TEST(CK3WorldProvinceHoldingTests, HoldingDuchyBuildingCanBeLoaded)  // NOLINT : clang-tidy doens't like gtest
{
   std::stringstream input;
   input << "holding = {\n";
   input << "\tbuildings = {\n";
   input << "\t\n{\ntype = \"castle_01\"}\n{type = \"hill_farms_02\"}\n";
   input << "\t}\n";
   input << "duchy_capital_building={disabled = yes type = tax_assessor_01}\n";
   input << "}";

   const ck3::ProvinceHolding holding(input);

   ASSERT_EQ(2, holding.GetBuildings().size());
   ASSERT_TRUE(holding.GetDuchyCapitalBuilding().has_value());
   if (holding.GetDuchyCapitalBuilding().has_value())
   {
      ASSERT_EQ(holding.GetDuchyCapitalBuilding()->GetType(), "tax_assessor");
      ASSERT_EQ(holding.GetDuchyCapitalBuilding()->GetLevel(), 1);
      ASSERT_TRUE(holding.GetDuchyCapitalBuilding()->IsDisabled());
   }
}
