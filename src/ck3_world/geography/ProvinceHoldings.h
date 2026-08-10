#ifndef CK3_PROVINCEHOLDINGS_H
#define CK3_PROVINCEHOLDINGS_H
#include "Parser.h"

namespace CK3
{
class ProvinceHolding;
class ProvinceHoldings: commonItems::parser
{
  public:
   ProvinceHoldings() = default;
   explicit ProvinceHoldings(std::istream& theStream);

   [[nodiscard]] const auto& getProvinceHoldings() const { return province_holdings_; }

  private:
   void registerKeys();

   std::map<int, std::shared_ptr<ProvinceHolding>> province_holdings_;
};
}  // namespace CK3

#endif  // CK3_PROVINCEHOLDINGS_H
