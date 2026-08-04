#ifndef CK3_DYNASTIES_H
#define CK3_DYNASTIES_H
#include "Parser.h"
#include "dynasty_parser.hpp"
#include "house_parser.hpp"

namespace ck3
{

class DynastiesMap: commonItems::parser
{
  public:
   DynastiesMap() = default;
   explicit DynastiesMap(std::istream& input_stream);

   [[nodiscard]] const auto& GetDynasties() const { return dynasties_; }
   [[nodiscard]] auto GetHouses() { return houses_; }

  private:
   void ParseDynasties(std::istream& input_stream);
   void ParseHouses(std::istream& input_stream);
   void ParseOnlyDynasties(std::istream& input_stream);

   std::map<long long, HouseParser> houses_;
   std::map<long long, DynastyParser> dynasties_;
};
}  // namespace ck3

#endif  // CK3_DYNASTIES_H
