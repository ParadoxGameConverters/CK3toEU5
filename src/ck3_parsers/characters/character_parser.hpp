#ifndef CK3_CHARACTER_PARSER_H
#define CK3_CHARACTER_PARSER_H

#include <vector>

#include "Date.h"
#include "Parser.h"
#include "character_realm_parser.hpp"
#include "src/ck3_parsers/cultures/culture_parser.hpp"

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

class CharacterParser: commonItems::parser  // NOLINT : issues with error handling in parser
{
  public:
   CharacterParser(std::istream& input_stream, long long character_id);

   [[nodiscard]] auto IsDead() const { return death_date_.has_value(); }
   [[nodiscard]] auto IsKnight() const { return knight_; }
   [[nodiscard]] auto IsFemale() const { return female_; }
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
   long long house_ = -1;
   std::vector<int> traits_;

   std::optional<long long> culture_;
   std::optional<long long> faith_;

   std::optional<long long> employer_;
   std::optional<long long> primary_spouse_;
   std::vector<long long> spouses_;

   std::optional<long long> suzerain_;  // Tributary overlord
   std::vector<long long> children_;
   std::vector<long long> concubines_;
   std::vector<long long> knights_;  // employed champions and commanders, not council staff

   std::vector<long long> claims_;
   std::optional<CharacterRealmParser> realm_;

   Skills skills_;
};
}  // namespace ck3

#endif  // CK3_CHARACTER_PARSER_H
