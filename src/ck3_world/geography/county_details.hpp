#ifndef CK3_COUNTYDETAILS_H
#define CK3_COUNTYDETAILS_H

#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace ck3
{
class CountyDetail;
class Cultures;
class Religions;
class CountyDetails
{
  public:
   CountyDetails() = default;
   explicit CountyDetails(std::istream& input_stream);

   [[nodiscard]] const auto& GetCountyDetails() const { return county_details_; }

   void LinkCultures(const Cultures& cultures);
   void LinkReligions(const Religions& religions);

  private:
   void ParseCountyDetails(std::istream& input_stream);

   std::map<std::string, std::shared_ptr<CountyDetail>> county_details_;
};
}  // namespace ck3

#endif  // CK3_COUNTYDETAILS_H
