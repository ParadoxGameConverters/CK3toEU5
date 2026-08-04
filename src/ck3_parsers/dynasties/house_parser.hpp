#ifndef CK3_HOUSE_H
#define CK3_HOUSE_H
#include "Parser.h"

namespace ck3
{

class HouseParser: commonItems::parser
{
  public:
   HouseParser() = default;
   HouseParser(std::istream& input_stream, long long house_id);
   [[nodiscard]] const auto& GetKey() const { return key_; }
   [[nodiscard]] const auto& GetName() const { return name_; }
   [[nodiscard]] const auto& GetLocalizedName() const { return localized_name_; }
   [[nodiscard]] const auto& GetPrefix() const { return prefix_; }
   [[nodiscard]] const auto& GetDynasty() const { return dynasty_; }
   [[nodiscard]] const auto& GetID() const { return house_id_; }
   [[nodiscard]] const auto& GetHouseHead() const { return house_head_; }

  private:
   void ParseHouse(std::istream& input_stream);

   long long house_id_ = -1;
   std::string key_;
   std::string name_;
   std::string localized_name_;
   long long dynasty_ = -1;
   std::string prefix_;
   std::optional<long long> house_head_;  // houses can have missing heads or dead people...
};
}  // namespace ck3

#endif  // CK3_HOUSE_H
