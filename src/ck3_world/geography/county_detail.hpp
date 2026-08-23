#ifndef CK3_COUNTY_DETAIL_H
#define CK3_COUNTY_DETAIL_H
#include <iostream>
#include <memory>

#include "Parser.h"
#include "src/ck3_world/id_pointer_pair.hpp"

namespace ck3
{
class Culture;
class Faith;
class CountyDetail
{
  public:
   CountyDetail() = default;
   CountyDetail(std::istream& input_stream, std::string county_key);

   [[nodiscard]] const auto& GetCountyKey() const { return county_key_; }
   [[nodiscard]] auto GetDevelopment() const { return development_; }
   [[nodiscard]] const auto& GetCulture() const { return culture_; }
   [[nodiscard]] const auto& GetFaith() const { return faith_; }
   [[nodiscard]] const auto& IsDeJureHRE() const { return de_jure_hre_; }

  private:
   void ParseCountyDetails(std::istream& input_stream);

   int development_ = 0;
   std::string county_key_;
   IdPointerPair<Culture> culture_;
   IdPointerPair<Faith> faith_;
   bool de_jure_hre_ = false;
};
}  // namespace ck3

#endif  // CK3_COUNTY_DETAIL_H
