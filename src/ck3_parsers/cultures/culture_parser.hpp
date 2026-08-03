#ifndef CK3_CULTURE_H
#define CK3_CULTURE_H
#include <set>

#include "external/commonItems/Color.h"
#include "external/commonItems/Parser.h"

namespace ck3
{


class CultureParser: commonItems::parser  // NOLINT : issues with error handling in parser
{
  public:
   CultureParser() = default;
   CultureParser(std::istream& input_stream, long long culture_id);

   [[nodiscard]] auto GetID() const { return culture_id_; }
   [[nodiscard]] auto IsDynamic() const { return dynamic_; }
   [[nodiscard]] const auto& GetLocalizedName() const { return localized_name_; }
   [[nodiscard]] const auto& GetName() const { return name_; }
   [[nodiscard]] const auto& GetNameLists() const { return name_set_; }
   [[nodiscard]] const auto& GetHeritage() const { return heritage_; }
   [[nodiscard]] const auto& GetLanguage() const { return language_; }
   //[[nodiscard]] const auto& getColor() const { return color_; }
   [[nodiscard]] const auto& GetTemplate() const { return culture_template_; }
   [[nodiscard]] const auto& GetEthos() const { return ethos_; }
   [[nodiscard]] const auto& GetTraditions() const { return traditions_; }
   [[nodiscard]] const auto& GetEra() const { return era_; }
   //[[nodiscard]] auto getInnovationCount() const { return innovationCount; }

   void SetDynamic(bool dynamic) { dynamic_ = dynamic; }

  private:
   void ParseCulture(std::istream& input_stream);

   long long culture_id_ = 0;
   bool dynamic_ = false;  // this culture is dynamic (hybrid/divergence) and has no vanilla template

   std::optional<std::string> culture_template_;  // this has data only for base ck3 cultures, like czech or german
   std::optional<std::string> localized_name_;    // this can be anything - user input or localized name in a
                                                  // particular language game is running.
   std::string heritage_;                         // all cultures should have this.
   std::string language_;                         // all cultures should have this. Used for EU5 language mapping.
   // std::optional<commonItems::Color> color_;		// the color the culture wore on the CK3 map
   std::set<std::string>
       name_set_;  // We use these to generate dynamic culture code names, in lack of a better solution.
   std::string ethos_;
   std::vector<std::string> traditions_;
   std::string era_;  // culture_era_tribal / _early_medieval / _high_medieval / _late_medieval
   // int innovationCount = 0; // innovations the culture has unlocked, across all eras

   std::string name_;  // calculated value: vanilla template name if present, otherwise localized/generated name.
};
}  // namespace ck3

#endif  // CK3_CULTURE_H
