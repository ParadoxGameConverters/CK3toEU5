#ifndef CK3_PROVINCE_HOLDINGS_H
#define CK3_PROVINCE_HOLDINGS_H

#include <map>
#include <memory>

#include "province_holding.hpp"

namespace ck3
{
class ProvinceHoldings
{
  public:
   ProvinceHoldings() = default;
   explicit ProvinceHoldings(std::istream& input_stream);

   [[nodiscard]] const auto& GetProvinceHoldings() const { return province_holdings_; }

  private:
   void ParseProvinceHoldings(std::istream& input_stream);

   std::map<int, std::shared_ptr<ProvinceHolding>> province_holdings_;
};
}  // namespace ck3

#endif  // CK3_PROVINCE_HOLDINGS_H
