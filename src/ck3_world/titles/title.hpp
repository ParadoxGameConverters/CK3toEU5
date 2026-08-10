#ifndef CK3_TITLE_H
#define CK3_TITLE_H

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "Date.h"
#include "src/ck3_world/id_pointer_pair.hpp"

namespace ck3
{
const date kNeverCreatedDate = date(9999, 1, 1); // NOLINT : bugprone exception (that won't happen)
enum Level : std::uint8_t
{
   kUnknown,
   kBarony,
   kCounty,
   kDuchy,
   kKingdom,
   kEmpire,
   kHegemony
};

class Character;

class Title
{
  public:
   Title(std::istream& input_stream, long long title_id);

   [[nodiscard]] auto GetID() const { return title_id_; }
   [[nodiscard]] auto IsElective() const { return !electors_.empty(); }
   [[nodiscard]] const auto& GetKey() const { return key_; }
   [[nodiscard]] const auto& GetName() const { return name_; }
   [[nodiscard]] const auto& GetCustomName() const { return custom_name_; }
   [[nodiscard]] const auto& GetAdjective() const { return adjective_; }
   [[nodiscard]] const auto& GetCreationDate() const { return last_holder_change_date_; }
   [[nodiscard]] const auto& GetHistoryGovernment() const { return history_government_; }
   [[nodiscard]] const auto& GetCapitalCounty() const { return capital_county_; }

   [[nodiscard]] const auto& GetHeirs() const { return heirs_; }
   [[nodiscard]] const auto& GetClaimants() const { return claimants_; }
   [[nodiscard]] const auto& GetElectors() const { return electors_; }
   [[nodiscard]] const auto& GetLaws() const { return laws_; }
   [[nodiscard]] const auto& GetHolder() const { return holder_; }

   [[nodiscard]] Level GetLevel() const { return level_; }

   [[nodiscard]] const auto& GetDeFactoLiege() const { return de_facto_liege_; }
   [[nodiscard]] const auto& GetDeJureLiege() const { return de_jure_liege_; }
   [[nodiscard]] const auto& GetDeJureVassals() const { return de_jure_vassals_; }

   [[nodiscard]] bool IsCountyCapital() const { return county_capital_barony_; }
   [[nodiscard]] bool IsDuchyCapital() const { return duchy_capital_barony_; }
   [[nodiscard]] auto IsTheocraticLease() const { return theocratic_lease_; }
   [[nodiscard]] auto IsLandless() const { return landless_; }

   [[nodiscard]] bool DoesTitleExist();

   void SetLevel(Level new_level) { level_ = new_level; }


  private:
   long long title_id_ = -1;
   std::string key_;                         // c_ashmaka - Immutable.
   std::string name_;                        // Ashmaka
   std::optional<std::string> custom_name_;  // "Malik City" - only present when the player manually renamed the title.
   std::string adjective_;                   // Ashmakan
   std::string history_government_;          // Unclear why this is called "history"
   date last_holder_change_date_;            // Date of the last holder change 9999.1.1 for never-held titles

   std::set<std::string> laws_;
   Level level_ = Level::kUnknown;

   IdPointerPair<Title> capital_county_;  // capital title is a COUNTY, even for county itself and baronies beneath it!

   std::optional<IdPointerPair<Character>> holder_;
   std::vector<IdPointerPair<Character>> heirs_;
   std::vector<IdPointerPair<Character>> claimants_;

   std::vector<IdPointerPair<Character>> electors_;

   std::optional<IdPointerPair<Title>> de_facto_liege_;
   std::optional<IdPointerPair<Title>> de_jure_liege_;

   std::vector<IdPointerPair<Title>> de_jure_vassals_;

   std::optional<IdPointerPair<Title>> county_details_;

   std::optional<IdPointerPair<Title>> holding_;

   bool theocratic_lease_ = false;
   bool landless_ = false;
   bool county_capital_barony_ = false;
   bool duchy_capital_barony_ = false;

   void ParseTitle(std::istream& input_stream);
   void DetermineLevelAfterParsing();
};

}  // namespace ck3

#endif  // CK3_TITLE_H