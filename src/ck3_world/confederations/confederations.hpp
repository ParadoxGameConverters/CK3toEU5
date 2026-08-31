#ifndef CK3_CONFEDERATIONS_H
#define CK3_CONFEDERATIONS_H
#include "Parser.h"
#include "confederation.hpp"

namespace ck3
{
class Characters;
class Dynasties;
class Confederations: commonItems::parser
{
  public:
   Confederations() = default;
   explicit Confederations(std::istream& input_stream);

   void LinkCharacters(const Characters& characters);
   void LinkHouses(const Dynasties& dynasties);

   [[nodiscard]] const auto& GetConfederations() const { return confederations_; }

  private:
   std::map<long long, std::shared_ptr<Confederation>> confederations_;

   void ParseConfederations(std::istream& input_stream);
};
}  // namespace ck3

#endif  // CK3_CONFEDERATIONS_H
