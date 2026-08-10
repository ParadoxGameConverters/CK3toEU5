#ifndef CK3_COUNTYDETAIL_H
#define CK3_COUNTYDETAIL_H
#include "Parser.h"
#include "src/ck3_world/religions/faith_parser.hpp"

namespace ck3
{

class CountyDetail: commonItems::parser
{
  public:
   CountyDetail() = default;
   CountyDetail(std::istream& theStream);

   [[nodiscard]] auto GetDevelopment() const { return development; }
   [[nodiscard]] const auto& GetCulture() const { return culture; }
   [[nodiscard]] const auto& GetFaith() const { return faith; }
   [[nodiscard]] const auto& IsDeJureHRE() const { return deJureHRE; }

  private:
   void registerKeys();

   int development = 0;
   std::pair<long long, std::shared_ptr<Culture>> culture;
   std::pair<long long, std::shared_ptr<FaithParser>> faith;
   bool deJureHRE = false;
};
}  // namespace ck3

#endif  // CK3_COUNTYDETAIL_H
