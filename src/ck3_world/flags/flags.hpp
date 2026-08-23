#ifndef CK3_FLAGS_H
#define CK3_FLAGS_H
#include <map>
#include <set>

#include "Parser.h"
#include "flag.hpp"

namespace ck3
{
class Flags: commonItems::parser
{
  public:
   Flags() = default;
   explicit Flags(std::istream& input_stream);

   [[nodiscard]] const auto& GetFlags() const { return flags_; }
   [[nodiscard]] const auto& GetUnavailableDecisionFlags() const { return unavailable_unique_decisions_; }

  private:
   void ParseVariables(std::istream& input_stream);
   void ParseDecisionFlag(std::istream& input_stream);

   std::map<std::string, Flag> flags_;
   std::set<std::string> unavailable_unique_decisions_;
};
}  // namespace ck3

#endif  // CK3_FLAGS_H
