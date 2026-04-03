#pragma once

#include "ck3/world.h"
#include "config/configuration.h"
#include "diagnostics/diagnostics_report.h"
#include "common/pds_node.h"

#include <string_view>

namespace ck3eu5::ck3 {

class WorldImporter
{
  public:
	World importText(std::string_view text, diagnostics::DiagnosticsReport& diagnostics) const;
	World importFromConfiguration(const config::Configuration& configuration, diagnostics::DiagnosticsReport& diagnostics) const;

  private:
	static void parseDynasties(const common::PdsNode& root, World& world);
	static void parseDynastyHouses(const common::PdsNode& root, World& world);
	static void parseCultures(const common::PdsNode& root, World& world);
	static void parseFaiths(const common::PdsNode& root, World& world);
	static void parseCharacters(const common::PdsNode& root, World& world);
	static void parseTitles(const common::PdsNode& root, World& world);
	static void parseWars(const common::PdsNode& root, World& world);
	static void parseCounties(const common::PdsNode& root, World& world);
	static void synthesizeCountiesFromTitles(World& world);
	static void backfillRelationships(World& world);
	static std::string deriveDisplayName(const std::string& key);
};

}  // namespace ck3eu5::ck3
