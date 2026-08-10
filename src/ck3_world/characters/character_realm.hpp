#ifndef CK3_CHARACTERDOMAIN_H
#define CK3_CHARACTERDOMAIN_H
#include <set>

#include "Date.h"
#include "Parser.h"
#include "src/ck3_world/id_pointer_pair.hpp"

namespace ck3
{
class Character;
class Title;
class CharacterRealm: commonItems::parser  // NOLINT : issues with error handling in parser
{
  public:
   explicit CharacterRealm(std::istream& input_stream);

   [[nodiscard]] auto GetVassalPower() const { return vassal_power_; }
   [[nodiscard]] const auto& GetGovernmentType() const { return government_type_; }
   [[nodiscard]] const auto& GetLaws() const { return laws_; }
   [[nodiscard]] const auto& GetRealmCapital() const { return realm_capital_; }
   [[nodiscard]] const auto& GetDomain() const { return domain_; }
   [[nodiscard]] const auto& GetCourtLanguage() const { return court_language_; }
   [[nodiscard]] const auto& GetCouncil() const { return council_; }

  private:
   void ParseLandedData(std::istream& input_stream);
   void ParseCourtData(std::istream& input_stream);

   std::optional<double> vassal_power_;
   std::string government_type_;
   std::set<std::string> laws_;
   std::string court_language_;

   IdPointerPair<Title> realm_capital_;  // A barony!
   std::vector<IdPointerPair<Title>>
       domain_;  // These are all titles_ owned (b-c-d-k-e), landless included. First one is PRIMARY.
   std::vector<IdPointerPair<Character>> council_;
};
}  // namespace ck3

#endif  // CK3_CHARACTERDOMAIN_H
