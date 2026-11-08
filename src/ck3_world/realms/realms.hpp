#ifndef CK3_REALMS_H
#define CK3_REALMS_H

#include <memory>
#include <vector>

#include "realm.hpp"

namespace ck3
{
class Titles;
class Characters;
class CountyDetails;

// Works out which of the save's titles are actually independent, and gathers each one's
// de facto land into a Realm. Sorted by county count, largest first.
class Realms
{
  public:
   Realms() = default;
   Realms(const Titles& titles, const Characters& characters, const CountyDetails& county_details);

   [[nodiscard]] const auto& GetRealms() const { return realms_; }

   // Dumps every realm to the log so a conversion can be eyeballed against the CK3 save.
   void LogRealmReport() const;

  private:
   std::vector<std::shared_ptr<Realm>> realms_;
};
}  // namespace ck3

#endif  // CK3_REALMS_H
