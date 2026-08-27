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
class CouncillorTask;
class CharacterRealm  // NOLINT(bugprone-exception-escape)
{
  public:
   explicit CharacterRealm(std::istream& input_stream);

   [[nodiscard]] auto GetVassalPower() const { return vassal_power_; }
   [[nodiscard]] const auto& GetGovernmentType() const { return government_type_; }
   [[nodiscard]] const auto& GetLaws() const { return laws_; }
   [[nodiscard]] auto& GetRealmCapital() { return realm_capital_; }
   [[nodiscard]] auto& GetDomain() { return domain_; }
   [[nodiscard]] const auto& GetCourtLanguage() const { return court_language_; }
   [[nodiscard]] auto& GetCouncil() { return council_; }

   void Link(const std::map<long long, std::shared_ptr<Title>>& id_title_map,
       const std::map<long long, std::shared_ptr<CouncillorTask>>& tasks,
       long long character_id);

  private:
   void ParseLandedData(std::istream& input_stream);
   void ParseCourtData(std::istream& input_stream);

   std::optional<double> vassal_power_;
   std::string government_type_;
   std::set<std::string> laws_;
   std::string court_language_;

   // A barony, not present for characters with just ceremonial titles
   std::optional<IdPointerPair<Title>> realm_capital_;
   // These are all titles_ owned (b-c-d-k-e), landless included. First one is PRIMARY.
   std::vector<IdPointerPair<Title>> domain_;
   std::vector<IdPointerPair<CouncillorTask>> council_;
};
}  // namespace ck3

#endif  // CK3_CHARACTERDOMAIN_H
