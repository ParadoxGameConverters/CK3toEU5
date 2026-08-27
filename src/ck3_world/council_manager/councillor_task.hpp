#ifndef CK3_COUNCILLOR_TASK_H
#define CK3_COUNCILLOR_TASK_H
#include <set>

#include "Parser.h"
#include "src/ck3_world/id_pointer_pair.hpp"

namespace ck3
{
class Character;
class CouncillorTask
{
  public:
   explicit CouncillorTask(std::istream& input_stream, long long task_id);

   [[nodiscard]] auto GetID() const { return task_id_; }
   [[nodiscard]] const auto& GetType() const { return type_; }
   [[nodiscard]] auto GetHolder() const { return holder_; }
   [[nodiscard]] auto GetCourtOwner() const { return court_owner_; }

   void LinkCharacters(const std::map<long long, std::shared_ptr<Character>>& characters);

  private:
   long long task_id_;
   std::string type_;
   std::optional<IdPointerPair<Character>> holder_;
   IdPointerPair<Character> court_owner_;
};
}  // namespace ck3

#endif  // CK3_COUNCILLOR_TASK_H
