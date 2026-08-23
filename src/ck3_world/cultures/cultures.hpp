#ifndef CK3_CULTURES_H
#define CK3_CULTURES_H
#include <memory>

#include "Parser.h"
#include "culture.hpp"

namespace ck3
{
class Cultures: commonItems::parser
{
  public:
   Cultures() = default;
   explicit Cultures(std::istream& input_stream);

   [[nodiscard]] const auto& GetCultures() const { return cultures_; }

  private:
   void ParseCultureManager(std::istream& input_stream);

   void ParseCultures(std::istream& input_stream);

   std::map<long long, std::shared_ptr<Culture>> cultures_;
};
}  // namespace ck3

#endif  // CK3_CULTURES_H
