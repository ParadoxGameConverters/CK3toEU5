#ifndef CK3_DYNASTIES_H
#define CK3_DYNASTIES_H
#include "Parser.h"
#include "dynasty.hpp"
#include "house.hpp"

namespace ck3
{

class Characters;
class Dynasties: commonItems::parser
{
  public:
   Dynasties() = default;
   explicit Dynasties(std::istream& input_stream);

   [[nodiscard]] const auto& GetDynasties() const { return dynasties_; }
   [[nodiscard]] const auto& GetHouses() const { return houses_; }

   void LinkCharacters(const Characters& characters);
   void LinkDynasties();

  private:
   void ParseDynasties(std::istream& input_stream);
   void ParseHouses(std::istream& input_stream);
   void ParseOnlyDynasties(std::istream& input_stream);

   std::map<long long, std::shared_ptr<House>> houses_;
   std::map<long long, std::shared_ptr<Dynasty>> dynasties_;
};
}  // namespace ck3

#endif  // CK3_DYNASTIES_H
