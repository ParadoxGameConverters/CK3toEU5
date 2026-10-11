#ifndef EU5_LOCATION_DATA_H
#define EU5_LOCATION_DATA_H

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace eu5
{

// A population group exactly as EU5 defines it in its own start data.
struct Pop
{
   std::string type;
   double size = 0.0;
   std::string culture;
   std::string religion;
};

// The culture and religion EU5 itself puts in each location, read from its own
// main_menu/setup/start/06_pops.txt.
//
// A generated country definition needs a culture_definition, and there is no CK3 culture to EU5
// culture mapping to produce one from - the configurables only cover culture groups. Taking the
// dominant culture EU5 already has in the country's capital gives a real EU5 culture key and lands
// somewhere geographically sensible.
class LocationData
{
  public:
   LocationData() = default;
   explicit LocationData(const std::filesystem::path& eu5_directory);

   // Empty when the location is unknown or held no pops.
   [[nodiscard]] std::string GetDominantCulture(const std::string& location) const;
   [[nodiscard]] std::string GetDominantReligion(const std::string& location) const;

   [[nodiscard]] auto GetLocationCount() const { return dominant_culture_.size(); }

   // Every location's pops, in file order, so they can be rewritten with converted religions while
   // keeping EU5's own types, sizes and cultures.
   [[nodiscard]] const auto& GetPops() const { return pops_; }

  private:
   void ParsePops(const std::filesystem::path& file_path);

   std::map<std::string, std::vector<Pop>> pops_;
   std::map<std::string, std::string> dominant_culture_;
   std::map<std::string, std::string> dominant_religion_;
};

}  // namespace eu5

#endif  // EU5_LOCATION_DATA_H
