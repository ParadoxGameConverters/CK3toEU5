#ifndef CK3_FAITH_H
#define CK3_FAITH_H
#include <set>

#include "Color.h"
#include "Parser.h"

namespace ck3
{
class FaithParser: commonItems::parser  // NOLINT : issues with error handling in parser
{
  public:
   FaithParser() = default;
   FaithParser(std::istream& input_stream, long long faith_id);

   [[nodiscard]] const auto& GetTag() const { return tag_; }
   [[nodiscard]] const auto& GetDoctrines() const { return doctrines_; }
   [[nodiscard]] const auto& GetReligion() const { return religion_; }
   [[nodiscard]] const auto& GetReligiousHead() const { return religious_head_; }
   [[nodiscard]] auto GetID() const { return faith_id_; }
   [[nodiscard]] const auto& GetCustomName() const { return custom_name_; }
   [[nodiscard]] const auto& GetCustomAdjective() const { return custom_adjective_; }
   [[nodiscard]] const auto& GetDescription() const { return description_; }
   [[nodiscard]] const auto& GetTemplate() const { return religion_template_; }
   [[nodiscard]] const auto& GetIconPath() const { return icon_path_; }
   [[nodiscard]] const auto& GetFaithType() const { return faith_type_; }
   [[nodiscard]] const auto& IsReformed() const { return is_reformed_; }

  private:
   void ParseFaith(std::istream& input_stream);
   void ParseDoctrine(std::istream& input_stream);

   bool is_reformed_ = true;

   long long faith_id_ = -1;
   std::string tag_;
   std::string religion_template_;
   std::string faith_type_;
   std::filesystem::path icon_path_;
   std::string custom_name_;
   std::string custom_adjective_;
   std::string description_;
   std::string religious_head_;
   std::set<std::string> doctrines_;
   std::set<std::string> tenets_;
   long long religion_ = -1;
};
}  // namespace ck3

#endif  // CK3_FAITH_H
