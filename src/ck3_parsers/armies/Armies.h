#ifndef CK3_ARMIES_H
#define CK3_ARMIES_H
#include <map>
#include <string>

#include "Parser.h"

namespace CK3
{
// Parses the savegame's armies block, keeping only men-at-arms regiments: entries in
// armies = { regiments = { id = { type = X size = N owner = charID } } } that carry a type.
// Untyped regiments are levies (raised from holdings, EU5 handles those natively through pops)
// and source=hired regiments are mercenary company stock owned by the company captain, not a realm.
class Armies: commonItems::parser
{
  public:
   Armies() = default;
   explicit Armies(std::istream& theStream);

   // owner character ID -> (ck3 men-at-arms type -> total men)
   [[nodiscard]] const auto& getMenAtArms() const { return menAtArms; }
   [[nodiscard]] int getRegimentCount() const { return regimentCount; }

  private:
   void registerKeys();

   std::map<long long, std::map<std::string, int>> menAtArms;
   int regimentCount = 0;
};
}  // namespace CK3

#endif  // CK3_ARMIES_H
