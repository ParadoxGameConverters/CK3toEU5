#ifndef CK3_RELIGION_H
#define CK3_RELIGION_H
#include "Parser.h"
#include "faith.hpp"
#include "src/ck3_world/id_pointer_pair.hpp"

namespace ck3
{

class Religion: commonItems::parser  // NOLINT : issues with error handling in parser
{
  public:
   Religion() = default;
   Religion(std::istream& input_stream, long long religion_id);

   [[nodiscard]] auto GetID() const { return religion_id_; }
   [[nodiscard]] const auto& GetTag() const { return tag_; }
   [[nodiscard]] const auto& GetFamily() const { return family_; }
   [[nodiscard]] const auto& GetFaiths() const { return faiths_; }
   [[nodiscard]] const auto& GetReligionType() const { return religion_type_; }

   void LinkFaiths(const std::map<long long, std::shared_ptr<Faith>>& faith_map);

  private:
   void ParseReligion(std::istream& input_stream);

   long long religion_id_ = -1;
   std::string religion_type_;
   std::string tag_;
   std::string family_;

   std::vector<IdPointerPair<Faith>> faiths_;
};
}  // namespace ck3

#endif  // CK3_RELIGION_H
