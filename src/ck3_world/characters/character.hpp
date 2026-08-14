#ifndef CK3_CHARACTER_H
#define CK3_CHARACTER_H

#include <vector>

#include "Date.h"
#include "Parser.h"
#include "character_realm.hpp"

namespace ck3
{

using Skills = struct Skills
{
   int diplomacy = 0;
   int martial = 0;
   int stewardship = 0;
   int intrigue = 0;
   int learning = 0;
   int prowess = 0;
};
class Faith;
class Culture;
class House;
class Title;
class Character
{
  public:
   Character(std::istream& input_stream, long long character_id);

   [[nodiscard]] auto IsDead() const { return death_date_.has_value(); }
   [[nodiscard]] auto IsKnight() const { return knight_; }
   [[nodiscard]] auto IsFemale() const { return female_; }
   [[nodiscard]] auto IsCouncilor() const { return councilor_; }
   [[nodiscard]] auto GetID() const { return character_id_; }
   [[nodiscard]] auto GetPiety() const { return piety_; }
   [[nodiscard]] auto GetPrestige() const { return prestige_; }
   [[nodiscard]] auto GetGold() const { return gold_; }
   [[nodiscard]] const auto& GetName() const { return name_; }
   [[nodiscard]] const auto& GetBirthDate() const { return birth_date_; }
   [[nodiscard]] const auto& GetDeathDate() const { return death_date_; }

   [[nodiscard]] const auto& GetCulture() const { return culture_; }
   [[nodiscard]] const auto& GetFaith() const { return faith_; }
   [[nodiscard]] const auto& GetEmployer() const { return employer_; }
   [[nodiscard]] const auto& GetSpouse() const { return primary_spouse_; }
   [[nodiscard]] const auto& GetSuzerain() const { return suzerain_; }
   [[nodiscard]] const auto& GetChildren() const { return children_; }
   [[nodiscard]] const auto& GetHouse() const { return house_; }
   [[nodiscard]] const auto& GetTraits() const { return traits_; }
   [[nodiscard]] const auto& GetClaims() const { return claims_; }

   [[nodiscard]] const auto& GetSkills() const { return skills_; }
   [[nodiscard]] const auto& GetCharacterRealm() const { return realm_; }
   [[nodiscard]] const auto& GetKnights() const { return knights_; }

  private:
   void ParseCharacter(std::istream& input_stream);
   void ParseDeadData(std::istream& input_stream);
   void ParseAliveData(std::istream& input_stream);
   static double RetrieveAccumulated(std::istream& input_stream);
   void ParseGold(std::istream& input_stream);
   void ParseCourtData(std::istream& input_stream);
   void ParsePlayableData(std::istream& input_stream);
   void ParseFamilyData(std::istream& input_stream);
   void ParseClaim(std::istream& input_stream);

   bool knight_ = false;
   bool female_ = false;
   bool councilor_ = false;
   long long character_id_ = 0;

   double piety_ = 0;
   double gold_ = 0;
   std::optional<double> influence_;
   std::optional<double> merit_;
   std::optional<double> prestige_;
   std::optional<double> treasury_;

   std::optional<double> legitimacy_;  // Only present with DLC's

   std::string name_;
   date birth_date_ = date("1.1.1");
   std::optional<date> death_date_;
   std::set<int> traits_;

   IdPointerPair<House> house_;

   std::optional<IdPointerPair<Culture>> culture_;
   std::optional<IdPointerPair<Faith>> faith_;

   std::optional<IdPointerPair<Character>> employer_;
   std::optional<IdPointerPair<Character>> primary_spouse_;
   std::vector<IdPointerPair<Character>> spouses_;

   std::optional<IdPointerPair<Character>> suzerain_;  // Tributary overlord
   std::vector<IdPointerPair<Character>> children_;
   std::vector<IdPointerPair<Character>> concubines_;
   std::vector<IdPointerPair<Character>> knights_;  // employed champions and commanders, not council staff

   std::vector<IdPointerPair<Title>> claims_;
   std::optional<CharacterRealm> realm_;

   Skills skills_;
};
}  // namespace ck3

#endif  // CK3_CHARACTER_PARSER_H
