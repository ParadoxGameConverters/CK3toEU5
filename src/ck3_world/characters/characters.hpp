#ifndef CK3_CHARACTERS_H
#define CK3_CHARACTERS_H
#include "Parser.h"
#include "character.hpp"

namespace ck3
{
class Cultures;
class Religions;
class Dynasties;
class Titles;
class CouncillorTasks;
class Characters
{
  public:
   Characters() = default;
   void ParseCharacters(std::istream& input_stream);

   [[nodiscard]] const auto& GetAliveCharacters() const { return characters_alive_; }
   [[nodiscard]] const auto& GetDeadCharacters() const { return characters_dead_; }
   [[nodiscard]] const auto& GetAllCharacters() const { return all_characters_; }

   void LinkCultures(const Cultures& cultures);
   void LinkFaiths(const Religions& religions);
   void LinkHouses(const Dynasties& houses);
   void LinkTitles(const Titles& titles, const CouncillorTasks& councillor_tasks);
   void LinkCharacters();

  private:
   std::map<long long, std::shared_ptr<Character>> characters_alive_;
   std::map<long long, std::shared_ptr<Character>> characters_dead_;
   std::map<long long, std::shared_ptr<Character>> all_characters_;
};
}  // namespace ck3

#endif  // CK3_CHARACTERS_H
