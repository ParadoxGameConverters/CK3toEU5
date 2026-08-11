#ifndef CK3_LANDED_TITLES_H
#define CK3_LANDED_TITLES_H

#include <filesystem>
#include <map>
#include <memory>
#include <string>

#include "landed_title.hpp"

namespace ck3
{
class LandedTitles
{
  public:
   void LoadTitles(const std::filesystem::path& file_name);

   [[nodiscard]] const auto& GetLandedTitles() const { return landed_titles_; }

  private:
   void ParseLandedTitle(std::istream& input_stream, const std::string& title_key);

   std::map<std::string, std::shared_ptr<LandedTitle>> landed_titles_;
};

}  // namespace ck3

#endif  // !CK3_LANDED_TITLES_H
