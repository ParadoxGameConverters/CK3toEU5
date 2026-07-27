#ifndef PROVINCE_MAPPER_H
#define PROVINCE_MAPPER_H
#include "Parser.h"
#include <map>
#include <vector>

namespace mappers
{
// Maps CK3 provinces (barony-level province IDs) to EU5 locations (names) and back.
// Sourced from configurables/province_mappings.txt which contains N:M links.
class ProvinceMapper: commonItems::parser
{
  public:
	ProvinceMapper() = default;
	explicit ProvinceMapper(std::istream& theStream);
	void loadProvinceMappings(const std::filesystem::path& fileName);

	[[nodiscard]] const std::vector<std::string>& getEU5Locations(long long ck3Province) const;
	[[nodiscard]] const std::vector<long long>& getCK3Provinces(const std::string& eu5Location) const;
	[[nodiscard]] auto getMappingCount() const { return mappingCount; }

  private:
	void registerKeys();

	std::map<long long, std::vector<std::string>> ck3ToEU5;
	std::map<std::string, std::vector<long long>> eu5ToCK3;
	int mappingCount = 0;

	static const std::vector<std::string> emptyLocations;
	static const std::vector<long long> emptyProvinces;
};
} // namespace mappers

#endif // PROVINCE_MAPPER_H
