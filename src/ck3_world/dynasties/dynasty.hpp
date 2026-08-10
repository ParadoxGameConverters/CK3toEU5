#ifndef CK3_DYNASTY_H
#define CK3_DYNASTY_H
#include "Parser.h"

namespace ck3
{

class Dynasty: commonItems::parser
{
  public:
   Dynasty() = default;
   Dynasty(std::istream& input_stream, long long savegame_dynasty_id);

   [[nodiscard]] auto GetSavegameDynastyID() const { return savegame_dynasty_id_; }
   [[nodiscard]] auto IsAppropriateRealmName() const { return appropriate_realm_name_; }
   [[nodiscard]] const auto& GetDynastyID() const { return dynasty_id_; }
   //[[nodiscard]] const auto& getCoA() const { return coa; }

  private:
   void ParseDynasty(std::istream& input_stream);

   std::string dynasty_id_;             // this is actual key, looks like an int but is actually string.
   long long savegame_dynasty_id_ = 0;  // this is savegame key
   bool appropriate_realm_name_ = false;
};
}  // namespace ck3

#endif  // CK3_DYNASTY_H
