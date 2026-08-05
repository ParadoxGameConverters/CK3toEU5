#ifndef CK3_CONFEDERATION_H
#define CK3_CONFEDERATION_H
#include "Color.h"
#include "Parser.h"

namespace ck3
{
class CoatOfArms;

// A standing bloc of houses - the steppe and Japanese confederations CK3 lets dynasties form.
// Membership is by house rather than by character, so the realms behind a bloc are found by
// looking up who each house's head rules.
class ConfederationParser: commonItems::parser
{
  public:
   ConfederationParser() = default;
   ConfederationParser(std::istream& input_stream, long long confederation_id);

   [[nodiscard]] auto GetID() const { return confederation_id_; }
   [[nodiscard]] auto GetLeaderHouse() const { return leader_house_; }
   [[nodiscard]] const auto& GetName() const { return name_; }
   //[[nodiscard]] const auto& getColor() const { return color; }
   //[[nodiscard]] const auto& getCoat() const { return coat; }
   [[nodiscard]] const auto& GetHouses() const { return houses_; }

  private:
   void ParseConfederation(std::istream& input_stream);


   long long confederation_id_ = -1;
   long long leader_house_ = -1;
   std::string name_;
   std::vector<long long> houses_;
};
}  // namespace ck3

#endif  // CK3_CONFEDERATION_H
