#ifndef CK3_DYNAMIC_TITLE_INFO_H
#define CK3_DYNAMIC_TITLE_INFO_H
#include <string>

#include "Parser.h"

namespace ck3
{
class DynamicTitleInfo
{
  public:
   explicit DynamicTitleInfo(std::istream& input_stream);

   [[nodiscard]] const auto& GetDynamicTitleKey() const { return dynamic_key_; }
   [[nodiscard]] const auto& GetDynamicTitleRank() const { return dynamic_rank_; }

  private:
   void ParseDynamicTitleInfo(std::istream& input_stream);

   std::string dynamic_key_;
   std::string dynamic_rank_;
};
}  // namespace ck3

#endif  // CK3_DYNAMIC_TITLE_INFO_H
