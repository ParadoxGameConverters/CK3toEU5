#ifndef EU5_CULTURE_RESOLVER_H
#define EU5_CULTURE_RESOLVER_H

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "eu5_game_definitions.hpp"

namespace ck3
{
class Culture;
}

namespace mappers
{
class CultureGroupMapper;
class LanguageMapper;
}  // namespace mappers

namespace eu5
{

// Decides what a CK3 culture means in EU5 terms.
//
// EU5 is roughly ten times more granular than CK3: there is no german culture, there are twenty
// seven of them. So a one to one mapping cannot be written, and flattening Westphalia, Swabia and
// the Palatinate into a single "german" would lose more than it converts.
//
// Instead EU5's own culture is kept wherever CK3 has nothing finer to say - that is, wherever the
// EU5 culture already belongs to a group or speaks a language the CK3 culture maps onto. Only land
// where the two genuinely disagree, because the player moved a culture somewhere it does not
// belong, is overwritten. Where CK3's culture has no EU5 equivalent at all - norse, and every
// hybrid or divergent culture a campaign invents - a definition is generated so it keeps its name
// rather than being forced into the nearest stranger.
class CultureResolver
{
  public:
   CultureResolver() = default;
   CultureResolver(const GameDefinitions& game_definitions,
       const mappers::CultureGroupMapper& culture_group_mapper,
       const mappers::LanguageMapper& language_mapper);

   // True when EU5's culture is a plausible finer grained member of the CK3 one, so EU5's detail
   // should stand. Also true when nothing is known about the CK3 culture, since overwriting on
   // ignorance is worse than leaving the game's own data alone.
   [[nodiscard]] bool IsCompatible(const ck3::Culture& ck3_culture, const std::string& eu5_culture) const;

   // The EU5 culture a CK3 culture should become, generating a definition if EU5 has no
   // equivalent. Empty when neither EU5 nor the configurables say enough to place it.
   [[nodiscard]] std::string Resolve(const ck3::Culture& ck3_culture);

   // Marks a generated culture as actually used, so unused definitions are not shipped.
   void MarkUsed(const std::string& eu5_culture);

   [[nodiscard]] std::map<std::string, CultureDefinition> GetUsedGeneratedCultures() const;
   [[nodiscard]] const auto& GetGeneratedCultures() const { return generated_cultures_; }
   [[nodiscard]] const auto& GetUnresolved() const { return unresolved_; }

  private:
   // The EU5 culture groups a CK3 culture maps onto, by heritage, then language, then name list.
   [[nodiscard]] std::vector<std::string> GroupsFor(const ck3::Culture& ck3_culture) const;
   [[nodiscard]] std::optional<std::string> LanguageFor(const ck3::Culture& ck3_culture) const;

   const GameDefinitions* game_definitions_ = nullptr;
   const mappers::CultureGroupMapper* culture_group_mapper_ = nullptr;
   const mappers::LanguageMapper* language_mapper_ = nullptr;

   // A language spoken by some existing EU5 culture in a group, for generated cultures whose own
   // language is unmapped.
   std::map<std::string, std::string> group_language_;

   std::map<std::string, std::string> resolved_;
   std::map<std::string, CultureDefinition> generated_cultures_;
   std::set<std::string> used_cultures_;
   std::set<std::string> unresolved_;
};

}  // namespace eu5

#endif  // EU5_CULTURE_RESOLVER_H
