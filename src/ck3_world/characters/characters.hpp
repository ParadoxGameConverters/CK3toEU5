#ifndef CK3_CHARACTERS_H
#define CK3_CHARACTERS_H
#include "Parser.h"
#include "character.hpp"

namespace ck3
{
class Characters
{
  public:
   Characters() = default;
   void ParseAliveCharacters(std::istream& input_stream);
   void ParseDeadCharacters(std::istream& input_stream);

   [[nodiscard]] const auto& GetAliveCharacters() const { return characters_alive_; }
   [[nodiscard]] const auto& GetDeadCharacters() const { return characters_dead_; }

   // void LinkCultures(const Cultures& cultures);
   // void LinkFaiths(const Faiths& faiths);
   // void LinkHouses(const Houses& houses);
   // void LinkTitles(const Titles& titles);
   // void LinkCharacters();
   // void LinkTraits(const mappers::TraitScraper& traitScraper);

  private:
   std::map<long long, std::shared_ptr<Character>> characters_alive_;
   std::map<long long, std::shared_ptr<Character>> characters_dead_;
};
}  // namespace ck3

#endif  // CK3_CHARACTERS_H
