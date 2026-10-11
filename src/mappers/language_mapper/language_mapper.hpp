#ifndef MAPPERS_LANGUAGE_MAPPER_H
#define MAPPERS_LANGUAGE_MAPPER_H

#include <filesystem>
#include <istream>
#include <map>
#include <optional>
#include <string>

namespace mappers
{

// Maps CK3 languages onto EU5 languages, from data/configurables/language_map.txt.
// One eu5 language per link, one or more ck3 languages, and optionally a ck3 name list that
// should resolve to the same EU5 language:
//    link = { eu5 = arabic_language ck3 = language_arabic name_list = name_list_levantine }
class LanguageMapper
{
  public:
   LanguageMapper() = default;
   explicit LanguageMapper(std::istream& input_stream);
   explicit LanguageMapper(const std::filesystem::path& file_path);

   [[nodiscard]] std::optional<std::string> GetEU5Language(const std::string& ck3_language) const;
   [[nodiscard]] std::optional<std::string> GetEU5LanguageForNameList(const std::string& ck3_name_list) const;
   [[nodiscard]] const auto& GetMappings() const { return ck3_language_to_eu5_language_; }
   [[nodiscard]] const auto& GetNameListMappings() const { return ck3_name_list_to_eu5_language_; }

  private:
   void ParseMappings(std::istream& input_stream);

   std::map<std::string, std::string> ck3_language_to_eu5_language_;
   std::map<std::string, std::string> ck3_name_list_to_eu5_language_;
};

}  // namespace mappers

#endif  // MAPPERS_LANGUAGE_MAPPER_H
