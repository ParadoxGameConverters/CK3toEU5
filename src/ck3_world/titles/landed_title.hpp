#ifndef CK3_LANDED_TITLE_H
#define CK3_LANDED_TITLE_H

#include <string>
#include <utility>

namespace ck3
{
class LandedTitle
{
  public:
   explicit LandedTitle(std::string title_key): title_key_(std::move(title_key)) {}

   [[nodiscard]] const auto& GetProvince() const { return province_; }
   [[nodiscard]] const auto& GetTitleKey() const { return title_key_; }
   // The title this one is nested under in landed_titles, empty for top level titles.
   [[nodiscard]] const auto& GetParentKey() const { return parent_key_; }
   [[nodiscard]] const auto& IsDefiniteForm() const { return definite_form_; }
   [[nodiscard]] const auto& IsLandless() const { return landless_; }
   [[nodiscard]] const auto& CanBeNamedAfterDynasty() const { return can_be_named_after_dynasty_; }
   [[nodiscard]] const auto& DoesRulerUseTitleName() const { return ruler_uses_title_name_; }

   void SetProvince(long long province) { province_ = province; }
   void SetParentKey(std::string parent_key) { parent_key_ = std::move(parent_key); }
   void SetDefiniteForm(bool definite_form) { definite_form_ = definite_form; }
   void SetLandless(bool landless) { landless_ = landless; }
   void SetCanBeNamedAfterDynasty(bool can_be_named_after_dynasty)
   {
      can_be_named_after_dynasty_ = can_be_named_after_dynasty;
   }
   void SetRulerUsesTitleName(bool ruler_uses_title_name) { ruler_uses_title_name_ = ruler_uses_title_name; }
   void SetTitleKey(const std::string& title_key) { title_key_ = title_key; }

  private:
   bool definite_form_ = false;
   bool landless_ = false;
   bool can_be_named_after_dynasty_ = false;
   bool ruler_uses_title_name_ = false;

   long long province_ = -1;
   std::string title_key_;
   std::string parent_key_;
};

}  // namespace ck3

#endif  // !CK3_LANDED_TITLE_H
