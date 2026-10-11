#ifndef CK3_REALM_H
#define CK3_REALM_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/ck3_world/titles/title.hpp"

namespace ck3
{
class Character;
class CountyDetail;

// An independent CK3 realm: a ruler, the top level titles they hold, and every county under them
// de facto - their own and their vassals'. This is the unit that becomes an EU5 country.
class Realm
{
  public:
   Realm(std::shared_ptr<Title> primary_title, std::shared_ptr<Character> holder);

   [[nodiscard]] const auto& GetPrimaryTitle() const { return primary_title_; }
   [[nodiscard]] const auto& GetHolder() const { return holder_; }
   [[nodiscard]] const auto& GetCapitalCounty() const { return capital_county_; }
   [[nodiscard]] const auto& GetHeldTitles() const { return held_titles_; }
   [[nodiscard]] const auto& GetCounties() const { return counties_; }

   [[nodiscard]] Level GetTier() const;
   [[nodiscard]] std::string GetRealmName() const;
   [[nodiscard]] std::string GetRulerName() const;
   [[nodiscard]] std::string GetCultureName() const;
   [[nodiscard]] std::string GetFaithName() const;
   [[nodiscard]] std::string GetGovernment() const;

   void SetCapitalCounty(std::shared_ptr<Title> capital) { capital_county_ = std::move(capital); }
   void SetCapitalDetails(std::shared_ptr<CountyDetail> details) { capital_details_ = std::move(details); }
   void AddHeldTitle(std::shared_ptr<Title> title) { held_titles_.emplace_back(std::move(title)); }
   void AddCounty(std::shared_ptr<Title> county) { counties_.emplace_back(std::move(county)); }

  private:
   std::shared_ptr<Title> primary_title_;
   std::shared_ptr<Character> holder_;
   std::shared_ptr<Title> capital_county_;

   // Historical rulers often carry no culture or faith of their own in the save, so the realm
   // capital's county data stands in for them.
   std::shared_ptr<CountyDetail> capital_details_;

   // Independent top level titles this ruler holds. Usually one, but a ruler can hold several.
   std::vector<std::shared_ptr<Title>> held_titles_;
   std::vector<std::shared_ptr<Title>> counties_;
};
}  // namespace ck3

#endif  // CK3_REALM_H
