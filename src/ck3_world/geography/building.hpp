#ifndef CK3_BUILDING_H
#define CK3_BUILDING_H

#include <string>

namespace ck3
{
class Building
{
  public:
   explicit Building(std::istream& input_stream);

   [[nodiscard]] const auto& GetType() const { return type_; }
   [[nodiscard]] auto IsDisabled() const { return disabled_; }
   [[nodiscard]] auto GetLevel() const { return level_; }

  private:
   std::string type_;
   bool disabled_ = false;
   int level_ = 0;

   void ParseBuilding(std::istream& input_stream);
};

}  // namespace ck3

#endif  // CK3_BUILDING_H