#ifndef CK3_LANDED_TITLES_H
#define CK3_LANDED_TITLES_H

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "landed_title.hpp"

namespace ck3
{
class LandedTitles
{
  public:
   void LoadTitles(const std::filesystem::path& file_name);
   void LoadTitles(std::istream& input_stream);

   [[nodiscard]] const auto& GetLandedTitles() const { return landed_titles_; }

   // Titles nested directly under the given one - a county's baronies, a duchy's counties. Empty
   // when the title has none. The save omits de_jure_vassals for many counties, so this game file
   // hierarchy is the only complete source of a county's baronies.
   [[nodiscard]] const std::vector<std::string>& GetChildren(const std::string& title_key) const;

  private:
   void ParseLandedTitle(std::istream& input_stream, const std::string& title_key, const std::string& parent_key);

   std::map<std::string, std::shared_ptr<LandedTitle>> landed_titles_;
   std::map<std::string, std::vector<std::string>> children_;
};

}  // namespace ck3

#endif  // !CK3_LANDED_TITLES_H
