#ifndef EU5_VANILLA_COUNTRIES_H
#define EU5_VANILLA_COUNTRIES_H

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace eu5
{

// A country exactly as EU5 writes it in its own 10_countries.txt, kept as raw text.
struct VanillaCountry
{
   std::string tag;
   std::string block;                // the whole TAG = { ... } block, verbatim
   std::set<std::string> locations;  // every location the block claims
};

// EU5's own starting countries.
//
// The converter replaces 10_countries.txt wholesale, which silently unowns everything CK3 does not
// cover - the Americas, Oceania, much of Siberia. EU5 then complains about buildings and markets in
// locations with no owner, and those regions have no inhabitants at all. Countries whose land the
// conversion never touches are carried over verbatim instead.
class VanillaCountries
{
  public:
   VanillaCountries() = default;
   explicit VanillaCountries(const std::filesystem::path& eu5_directory);

   [[nodiscard]] const auto& GetCountries() const { return countries_; }

  private:
   void Parse(const std::filesystem::path& file_path);

   std::vector<VanillaCountry> countries_;
};

}  // namespace eu5

#endif  // EU5_VANILLA_COUNTRIES_H
