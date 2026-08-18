#ifndef CK3_PROVINCE_HOLDING_H
#define CK3_PROVINCE_HOLDING_H
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "Parser.h"
#include "building.hpp"

namespace ck3
{
class ProvinceHolding
{
  public:
   explicit ProvinceHolding(std::istream& input_stream);

   [[nodiscard]] const auto& GetHoldingType() const { return holding_type_; }
   [[nodiscard]] const auto& GetBuildings() const { return buildings_; }
   [[nodiscard]] const auto& GetSpecialBuilding() const { return special_building_; }
   [[nodiscard]] const auto& GetDuchyCapitalBuilding() const { return duchy_capital_building_; }
   [[nodiscard]] const auto& GetIncome() const { return income_; }
   [[nodiscard]] const auto& GetBarterGoods() const { return barter_goods_; }

  private:
   void ParseHolding(std::istream& input_stream);
   void ParseBuilding(std::istream& input_stream);

   std::string holding_type_;
   std::vector<Building> buildings_;
   std::string special_building_;
   std::optional<Building> duchy_capital_building_;
   // Be careful with this income - not every holding will have it and it might depend on holders skill
   double income_ = 0;
   double barter_goods_ = 0;
};
}  // namespace ck3

#endif  // CK3_PROVINCE_HOLDING_H
