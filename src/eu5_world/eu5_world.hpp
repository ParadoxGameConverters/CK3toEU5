#ifndef EU5_WORLD_H
#define EU5_WORLD_H

#include <Date.h>

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "eu5_country.hpp"
#include "eu5_culture_resolver.hpp"
#include "eu5_game_definitions.hpp"
#include "eu5_location_data.hpp"

namespace ck3
{
class CK3World;
class Culture;
class Realm;
class Title;
}

namespace mappers
{
class Mappers;
}

namespace eu5
{

// A CK3 vassal relationship, written out as an EU5 subject.
struct Dependency
{
   std::string liege_tag;
   std::string vassal_tag;
};

// Turns the independent CK3 realms into EU5 countries: a tag, a capital, and the EU5 locations the
// realm holds, walked down from its counties through their baronies.
class EU5World
{
  public:
   EU5World() = default;
   EU5World(const ck3::CK3World& ck3_world,
       const mappers::Mappers& mappers,
       const GameDefinitions& game_definitions,
       const LocationData& location_data);

   [[nodiscard]] const auto& GetCountries() const { return countries_; }
   [[nodiscard]] auto GetRealmsWithoutTag() const { return realms_without_tag_; }
   [[nodiscard]] auto GetCountiesWithoutBaronies() const { return counties_without_baronies_; }
   // The CK3 save's date, so converted rulers can be aged onto EU5's start date instead of being
   // born five centuries before the campaign begins.
   [[nodiscard]] const auto& GetConversionDate() const { return conversion_date_; }
   [[nodiscard]] const auto& GetLocationReligions() const { return location_religions_; }
   [[nodiscard]] const auto& GetLocationCultures() const { return location_cultures_; }
   [[nodiscard]] const auto& GetCultureResolver() const { return culture_resolver_; }
   [[nodiscard]] const auto& GetDependencies() const { return dependencies_; }
   [[nodiscard]] const auto& GetDevelopmentBonuses() const { return development_bonuses_; }

   void LogReport() const;

  private:
   // Vassals cannot outrank their liege, so ranks are settled after every country exists.
   void AssignRanks();
   // Carries CK3 development onto EU5 and derives a technology level from it.
   void AssignDevelopment();

   // The steps each CK3 ruler goes through on its way to becoming a country, in the order the
   // constructor runs them.
   struct Context;
   struct TagChoice
   {
      std::string tag;
      bool generated = false;
   };
   // What a CK3 county carries onto every EU5 location made from it.
   struct CountyData
   {
      std::string religion;
      int development = -1;
      std::shared_ptr<ck3::Culture> culture;
   };
   struct PopulationWeights
   {
      std::map<std::string, double> culture;
      std::map<std::string, double> religion;
   };

   [[nodiscard]] static std::optional<std::string> ResolveCapitalLocation(const ck3::Realm& realm,
       const Context& context);
   [[nodiscard]] std::optional<TagChoice> ChooseTag(const ck3::Realm& realm,
       const std::optional<std::string>& capital_location,
       Context& context);
   void MarkForDefinition(Country& country, const TagChoice& tag_choice, const Context& context);
   void AssignCapitalFaithAndCulture(Country& country,
       const ck3::Realm& realm,
       const std::optional<std::string>& capital_location,
       const Context& context);
   void AddCounty(Country& country, const ck3::Title& county, Context& context);
   [[nodiscard]] static CountyData ReadCountyData(const ck3::Title& county, const Context& context);
   void ClaimLocation(Country& country,
       const std::string& location,
       const CountyData& county_data,
       const Context& context);
   [[nodiscard]] PopulationWeights WeighPopulation(const Country& country, const Context& context) const;
   void AssignPopulationFaithAndCulture(Country& country, const Context& context);

   void LogLandReport() const;
   void LogTagReport() const;
   void LogFaithAndCultureReport() const;
   void LogCountryList() const;

   std::vector<std::shared_ptr<Country>> countries_;
   date conversion_date_ = date("1.1.1");
   // EU5 location to the religion of the CK3 county it was converted from.
   std::map<std::string, std::string> location_religions_;
   // EU5 location to the culture converted from the CK3 county, where the two disagree.
   std::map<std::string, std::string> location_cultures_;
   CultureResolver culture_resolver_;
   std::vector<Dependency> dependencies_;
   // EU5 location to the CK3 development of the county it came from, and the bonus that becomes.
   std::map<std::string, int> location_development_;
   std::map<std::string, int> development_bonuses_;
   int development_baseline_ = 0;

   std::size_t realms_without_tag_ = 0;
   std::size_t counties_without_baronies_ = 0;
   std::size_t landless_counties_ = 0;
   std::size_t religions_replaced_ = 0;
   std::size_t cultures_replaced_ = 0;
   std::size_t realms_with_generated_tag_ = 0;
   std::size_t duplicate_tags_regenerated_ = 0;
   std::size_t vassals_demoted_ = 0;
   std::set<std::string> undefined_tags_;
};

}  // namespace eu5

#endif  // EU5_WORLD_H
