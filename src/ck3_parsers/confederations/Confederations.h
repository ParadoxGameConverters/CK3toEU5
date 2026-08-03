#ifndef CK3_CONFEDERATIONS_H
#define CK3_CONFEDERATIONS_H
#include "Parser.h"

namespace CK3
{
class CoatsOfArms;
class Confederation;

// Parses the savegame's confederation_manager. Disbanded blocs keep their slot as "id=none".
class Confederations: commonItems::parser
{
  public:
   Confederations() = default;
   explicit Confederations(std::istream& theStream);

   void linkCoats(const CoatsOfArms& coats);

   [[nodiscard]] const auto& getConfederations() const { return confederations; }

  private:
   void registerKeys();
   parser databaseParser;

   std::map<long long, std::shared_ptr<Confederation>> confederations;
};
}  // namespace CK3

#endif  // CK3_CONFEDERATIONS_H
