#ifndef CK3_RELIGIONS_H
#define CK3_RELIGIONS_H

#include <memory>

#include "Parser.h"
#include "faith.hpp"
#include "religion.hpp"

namespace ck3
{

class Religions: commonItems::parser  // NOLINT : issues with error handling in parser
{
  public:
   Religions() = default;
   explicit Religions(std::istream& input_stream);

   [[nodiscard]] const auto& GetReligions() const { return religions_; }
   [[nodiscard]] auto GetFaiths() { return faiths_; }

  private:
   void ParseReligions(std::istream& input_stream);
   void ParseFaiths(std::istream& input_stream);
   void ParseOnlyReligions(std::istream& input_stream);

   std::map<long long, std::shared_ptr<Religion>> religions_;
   std::map<long long, std::shared_ptr<Faith>> faiths_;
};
}  // namespace ck3

#endif  // CK3_RELIGIONS_H
