#ifndef CK3_RELIGION_H
#define CK3_RELIGION_H
#include "Parser.h"

namespace ck3
{

class ReligionParser: commonItems::parser  // NOLINT : issues with error handling in parser
{
  public:
   ReligionParser() = default;
   ReligionParser(std::istream& input_stream, long long religion_id);

   [[nodiscard]] auto GetID() const { return religion_id_; }
   [[nodiscard]] const auto& GetTag() const { return tag_; }
   [[nodiscard]] const auto& GetFamily() const { return family_; }
   [[nodiscard]] const auto& GetFaiths() const { return faiths_; }
   [[nodiscard]] const auto& GetReligionType() const { return religion_type_; }

  private:
   void ParseReligion(std::istream& input_stream);

   long long religion_id_ = 0;
   std::string religion_type_;
   std::string tag_;
   std::string family_;
   std::set<long long> faiths_;
};
}  // namespace ck3

#endif  // CK3_RELIGION_H
