#ifndef OUTPUT_BLOCK_PARSING_H
#define OUTPUT_BLOCK_PARSING_H
#include "src/eu5_world/LocationDefinitions.h"
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

// Surgery on vanilla EU5 setup files. The converter keeps everything the CK3 map does not cover,
// which means reading vanilla's own script, deciding entry by entry what survives, and rewriting
// the parts that only partly survive.
namespace output
{
std::string slurpFile(const std::filesystem::path& file);

// Extracts top-level "name = { ... }" entries from a brace-delimited body, starting right after its
// opening brace. Comments are skipped. Returns pairs of (name, verbatim block text including the name).
std::vector<std::pair<std::string, std::string>> extractNamedBlocks(const std::string& text, size_t bodyStart);

// Finds the body start of "name = {" in text (position right after the brace), or npos.
size_t findBlockBody(const std::string& text, const std::string& name, size_t from = 0);

// The contents of "name = { ... }" in text, braces excluded and nesting respected, or empty when
// there is no such block.
std::string extractBlockBody(const std::string& text, const std::string& name, size_t from = 0);

// Whether a vanilla country entry claims any land the conversion took over. Entries may list
// areas/provinces/regions instead of raw locations; those get expanded.
bool touchesConvertedLand(const std::string& countryBlock, const std::set<std::string>& convertedLocations, const EU5::LocationDefinitions& definitions);

// Rewrites a vanilla country entry so it only claims land the conversion left it. Countries whose
// realm merely brushes the CK3 map - a Swahili sultanate with one converted port, a Siberian khanate
// half-covered by a CK3 empire - survive on what remains instead of vanishing and leaving their
// locations ownerless (which strands their towns' buildings with nobody to pay for them).
// Returns nullopt when the country cannot survive: its capital converted, or it kept no land at all.
std::optional<std::string> trimToSurvivingLand(const std::string& countryBlock,
	 const std::set<std::string>& convertedLocations,
	 const EU5::LocationDefinitions& definitions);
} // namespace output

#endif // OUTPUT_BLOCK_PARSING_H
