#ifndef CK3_CULTURES_H
#define CK3_CULTURES_H
#include "Parser.h"
#include "culture_parser.hpp"

namespace ck3
{
class CultureParserMap: commonItems::parser
{
  public:
   CultureParserMap() = default;
   explicit CultureParserMap(std::istream& input_stream);

   [[nodiscard]] const auto& GetCultures() const { return cultures_; }

  private:
   void ParseCultureManager(std::istream& input_stream);

   void ParseCultures(std::istream& input_stream);

   std::map<long long, CultureParser> cultures_;
};
}  // namespace ck3

#endif  // CK3_CULTURES_H
