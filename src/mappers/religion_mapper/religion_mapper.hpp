#ifndef MAPPERS_RELIGION_MAPPER_H
#define MAPPERS_RELIGION_MAPPER_H

#include <filesystem>
#include <istream>
#include <map>
#include <optional>
#include <string>

namespace mappers
{

// Maps CK3 faiths onto EU5 religions, from data/configurables/religion_map.txt.
// A link carries exactly one eu5 religion and one or more ck3 faiths:
//    link = { eu5 = mahayana ck3 = mahayana ck3 = ari ck3 = avatamsaka }
// Unresolved entries in that file are commented out with a ??? placeholder, so anything this
// mapper does not know about is a faith nobody has mapped yet.
class ReligionMapper
{
  public:
   ReligionMapper() = default;
   explicit ReligionMapper(std::istream& input_stream);
   explicit ReligionMapper(const std::filesystem::path& file_path);

   [[nodiscard]] std::optional<std::string> GetEU5Religion(const std::string& ck3_faith) const;
   [[nodiscard]] const auto& GetMappings() const { return ck3_faith_to_eu5_religion_; }

  private:
   void ParseMappings(std::istream& input_stream);

   std::map<std::string, std::string> ck3_faith_to_eu5_religion_;
};

}  // namespace mappers

#endif  // MAPPERS_RELIGION_MAPPER_H
