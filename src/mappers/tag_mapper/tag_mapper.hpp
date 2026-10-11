#ifndef MAPPERS_TAG_MAPPER_H
#define MAPPERS_TAG_MAPPER_H

#include <filesystem>
#include <istream>
#include <map>
#include <optional>
#include <string>

namespace mappers
{

// Maps CK3 titles onto EU5 country tags, from data/configurables/tag_mappings.txt.
//
//    link = { ck3 = k_denmark eu5 = DAN }
//    link = { capitals = { moscow } eu5 = MOS }
//
// A link matches either on the CK3 title key or on the realm's EU5 capital location. Per the file's
// own header, a capital match takes precedence over a title match: e_mongols with a capital in
// keflavik should become VST rather than the Mongol tag.
class TagMapper
{
  public:
   TagMapper() = default;
   explicit TagMapper(std::istream& input_stream);
   explicit TagMapper(const std::filesystem::path& file_path);

   // capital_location is the EU5 location the realm capital maps to, when one is known.
   [[nodiscard]] std::optional<std::string> GetEU5Tag(const std::string& ck3_title_key,
       const std::optional<std::string>& capital_location = std::nullopt) const;

   [[nodiscard]] const auto& GetTitleMappings() const { return title_to_tag_; }
   [[nodiscard]] const auto& GetCapitalMappings() const { return capital_to_tag_; }

  private:
   void ParseMappings(std::istream& input_stream);

   std::map<std::string, std::string> title_to_tag_;
   std::map<std::string, std::string> capital_to_tag_;
};

}  // namespace mappers

#endif  // MAPPERS_TAG_MAPPER_H
