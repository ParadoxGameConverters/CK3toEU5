#ifndef CK3_CONFEDERATION_H
#define CK3_CONFEDERATION_H
#include "Color.h"
#include "Parser.h"
#include "src/ck3_world/dynasties/house.hpp"
#include "src/ck3_world/id_pointer_pair.hpp"

namespace ck3
{

// A standing bloc of houses - the steppe and Japanese confederations CK3 lets dynasties form.
// Membership is by house rather than by character, so the realms behind a bloc are found by
// looking up who each house's head rules.
class Confederation: commonItems::parser
{
  public:
   Confederation() = default;
   Confederation(std::istream& input_stream, long long confederation_id);

   [[nodiscard]] auto GetID() const { return confederation_id_; }
   [[nodiscard]] auto GetLeaderHouse() const { return leader_house_; }
   [[nodiscard]] const auto& GetName() const { return name_; }
   //[[nodiscard]] const auto& getColor() const { return color; }
   //[[nodiscard]] const auto& getCoat() const { return coat; }
   [[nodiscard]] const auto& GetHouses() const { return houses_; }
   [[nodiscard]] const auto& GetMembers() const { return members_; }


  private:
   void ParseConfederation(std::istream& input_stream);

   long long confederation_id_ = -1;
   std::string name_;

   IdPointerPair<House> leader_house_;
   std::vector<IdPointerPair<House>> houses_;
   std::vector<IdPointerPair<Character>> members_;
};
}  // namespace ck3

#endif  // CK3_CONFEDERATION_H
