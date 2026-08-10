#ifndef CK3_FLAG_H
#define CK3_FLAG_H
#include <set>
#include <string>

#include "Parser.h"

namespace ck3
{
class Flag: commonItems::parser
{
  public:
   explicit Flag(std::istream& input_stream);

   [[nodiscard]] const auto& GetValue() const { return value_; }
   [[nodiscard]] const auto& GetName() const { return name_; }
   [[nodiscard]] const auto& GetType() const { return type_; }

  private:
   void ParseFlag(std::istream& input_stream);
   void ParseData(std::istream& input_stream);

   std::string name_;
   std::string type_;
   std::string value_;
};
}  // namespace ck3

#endif  // CK3_FLAG_H
