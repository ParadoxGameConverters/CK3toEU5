#ifndef CK3_TITLES_H
#define CK3_TITLES_H
#include <map>
#include <memory>
#include <string>

#include "Parser.h"

namespace ck3
{
class Title;
class Titles: commonItems::parser
{
  public:
   Titles() = default;
   explicit Titles(std::istream& input_stream);

   [[nodiscard]] const auto& GetTitles() const { return all_titles_; }
   [[nodiscard]] const auto& GetBaronies() const { return baronies_; }
   [[nodiscard]] const auto& GetCounties() const { return counties_; }
   [[nodiscard]] const auto& GetDuchies() const { return duchies_; }
   [[nodiscard]] const auto& GetKingdoms() const { return kingdoms_; }
   [[nodiscard]] const auto& GetEmpires() const { return empires_; }
   [[nodiscard]] const auto& GetHegemonies() const { return hegemonies_; }

  private:
   void ParseTitles(std::istream& input_stream);
   void ParseLandedTitles(std::istream& input_stream);
   void InsertToCorrectMap(std::shared_ptr<Title> new_title);
   void TranscribeDynamicTitleRanks();

   std::map<std::string, std::shared_ptr<Title>> all_titles_;  // We use key instead of id

   std::map<std::string, std::shared_ptr<Title>> baronies_;
   std::map<std::string, std::shared_ptr<Title>> counties_;
   std::map<std::string, std::shared_ptr<Title>> duchies_;
   std::map<std::string, std::shared_ptr<Title>> kingdoms_;
   std::map<std::string, std::shared_ptr<Title>> empires_;
   std::map<std::string, std::shared_ptr<Title>> hegemonies_;

   std::map<std::string, std::string> dynamic_titles_ranks_;
};
}  // namespace ck3

#endif  // CK3_TITLES_H
