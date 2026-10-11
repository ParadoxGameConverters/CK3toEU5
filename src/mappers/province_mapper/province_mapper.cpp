#include "province_mapper.hpp"

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "CommonRegexes.h"
#include "Log.h"
#include "Parser.h"
#include "ParserHelpers.h"

namespace
{
const std::vector<std::string> kNoLocations;
const std::vector<long long> kNoProvinces;

struct ProvinceLink
{
   std::vector<long long> ck3_provinces;
   std::vector<std::string> eu5_locations;
};

ProvinceLink ParseLink(std::istream& input_stream)
{
   ProvinceLink link;
   commonItems::parser link_parser;
   link_parser.registerKeyword("ck3", [&link](std::istream& input_stream) {
      link.ck3_provinces.emplace_back(commonItems::getLlong(input_stream));
   });
   link_parser.registerKeyword("eu5", [&link](std::istream& input_stream) {
      link.eu5_locations.emplace_back(commonItems::getString(input_stream));
   });
   link_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   link_parser.parseStream(input_stream);
   link_parser.clearRegisteredKeywords();
   return link;
}
}  // namespace

mappers::ProvinceMapper::ProvinceMapper(std::istream& input_stream)
{
   ParseMappings(input_stream);
}

mappers::ProvinceMapper::ProvinceMapper(const std::filesystem::path& file_path)
{
   std::ifstream file_stream(file_path);
   if (!file_stream.is_open())
   {
      throw std::runtime_error("Could not open " + file_path.string() + " for province mappings!");
   }
   ParseMappings(file_stream);
   Log(LogLevel::Info) << "<> Loaded " << ck3_to_eu5_.size() << " CK3 province mappings covering " << eu5_to_ck3_.size()
                       << " EU5 locations.";
}

void mappers::ProvinceMapper::ParseMappings(std::istream& input_stream)
{
   commonItems::parser parser;
   // The file is wrapped in a version block. Only the first is used.
   parser.registerRegex(R"([0-9\.]+)", [this](const std::string&, std::istream& input_stream) {
      if (ck3_to_eu5_.empty())
      {
         ParseVersion(input_stream);
      }
      else
      {
         commonItems::ignoreItem("version", input_stream);
      }
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

void mappers::ProvinceMapper::ParseVersion(std::istream& input_stream)
{
   commonItems::parser version_parser;
   version_parser.registerKeyword("link", [this](std::istream& input_stream) {
      const auto link = ParseLink(input_stream);
      // Deliberate one sided entries exist - a CK3 wasteland with no EU5 counterpart, or an EU5
      // location with nothing in CK3. Neither is a usable mapping.
      if (link.ck3_provinces.empty() || link.eu5_locations.empty())
      {
         return;
      }
      for (const auto ck3_province: link.ck3_provinces)
      {
         auto& locations = ck3_to_eu5_[ck3_province];
         locations.insert(locations.end(), link.eu5_locations.begin(), link.eu5_locations.end());
      }
      for (const auto& eu5_location: link.eu5_locations)
      {
         auto& provinces = eu5_to_ck3_[eu5_location];
         provinces.insert(provinces.end(), link.ck3_provinces.begin(), link.ck3_provinces.end());
      }
   });
   version_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   version_parser.parseStream(input_stream);
   version_parser.clearRegisteredKeywords();
}

const std::vector<std::string>& mappers::ProvinceMapper::GetEU5Locations(long long ck3_province) const
{
   const auto mapping = ck3_to_eu5_.find(ck3_province);
   return mapping == ck3_to_eu5_.end() ? kNoLocations : mapping->second;
}

const std::vector<long long>& mappers::ProvinceMapper::GetCK3Provinces(const std::string& eu5_location) const
{
   const auto mapping = eu5_to_ck3_.find(eu5_location);
   return mapping == eu5_to_ck3_.end() ? kNoProvinces : mapping->second;
}
