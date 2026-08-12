#ifndef CK3_BUILDING_H
#define CK3_BUILDING_H

#include <string>

namespace ck3
{
class Building
{
  public:
   explicit Building(std::istream& input_stream);

  private:
   std::string type_;
   bool disabled_ = false;

   void ParseBuilding(std::istream& input_stream);
};

}  // namespace ck3

#endif  // CK3_BUILDING_H