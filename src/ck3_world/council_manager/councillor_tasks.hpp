#ifndef CK3_COUNCILLOR_TASKS_H
#define CK3_COUNCILLOR_TASKS_H
#include <memory>

#include "Parser.h"
#include "councillor_task.hpp"

namespace ck3
{
class Characters;
class CouncillorTasks
{
  public:
   CouncillorTasks() = default;
   explicit CouncillorTasks(std::istream& input_stream);

   [[nodiscard]] const auto& GetCouncillorTasks() const { return tasks_; }

   void LinkCharacters(const Characters& characters);

  private:
   void ParseActive(std::istream& input_stream);

   std::map<long long, std::shared_ptr<CouncillorTask>> tasks_;
};
}  // namespace ck3

#endif  // CK3_COUNCILLOR_TASKS_H
