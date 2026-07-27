#include "Artifacts.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"
#include <sstream>

namespace
{
// One entry of an artifact's history: who did what to it, and when.
class HistoryEntry: commonItems::parser
{
  public:
	explicit HistoryEntry(std::istream& theStream)
	{
		registerKeyword("type", [this](std::istream& stream) {
			type = commonItems::getString(stream);
		});
		registerKeyword("date", [this](std::istream& stream) {
			when = date(commonItems::getString(stream));
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	std::string type;
	std::optional<date> when;
};

class ArtifactHistory: commonItems::parser
{
  public:
	explicit ArtifactHistory(std::istream& theStream)
	{
		registerKeyword("entries", [this](std::istream& stream) {
			for (const auto& blob: commonItems::blobList(stream).getBlobs())
			{
				std::stringstream blobStream(blob);
				const HistoryEntry entry(blobStream);
				if (!entry.when)
					continue;
				if (entry.type == "created")
					created = entry.when;
				if (!earliest || *entry.when < *earliest)
					earliest = entry.when;
			}
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	// Not every artifact records its creation - loot and gifts from before the save's history can
	// start mid-story - so the oldest entry stands in when there's no creation on record.
	[[nodiscard]] std::optional<date> getCreationDate() const { return created ? created : earliest; }

  private:
	std::optional<date> created;
	std::optional<date> earliest;
};

class ArtifactEntry: commonItems::parser
{
  public:
	explicit ArtifactEntry(std::istream& theStream)
	{
		registerKeyword("name", [this](std::istream& stream) {
			artifact.name = CK3::stripSaveMarkup(commonItems::getString(stream));
		});
		registerKeyword("description", [this](std::istream& stream) {
			artifact.description = CK3::stripSaveMarkup(commonItems::getString(stream));
		});
		registerKeyword("history", [this](std::istream& stream) {
			artifact.creationDate = ArtifactHistory(stream).getCreationDate();
		});
		registerKeyword("rarity", [this](std::istream& stream) {
			artifact.rarity = commonItems::getString(stream);
		});
		registerKeyword("owner", [this](std::istream& stream) {
			artifact.owner = commonItems::getLlong(stream);
		});
		registerKeyword("quality", [this](std::istream& stream) {
			artifact.quality = commonItems::getInt(stream);
		});
		registerKeyword("wealth", [this](std::istream& stream) {
			artifact.wealth = commonItems::getInt(stream);
		});
		registerKeyword("visuals", [this](std::istream& stream) {
			commonItems::parser visualsParser;
			visualsParser.registerKeyword("type", [this](std::istream& typeStream) {
				artifact.visualType = commonItems::getString(typeStream);
			});
			visualsParser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
			visualsParser.parseStream(stream);
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
		parseStream(theStream);
		clearRegisteredKeywords();
	}

	CK3::Artifact artifact;
};
} // namespace

CK3::Artifacts::Artifacts(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void CK3::Artifacts::registerKeys()
{
	registerKeyword("artifacts", [this](std::istream& theStream) {
		commonItems::parser registryParser;
		registryParser.registerRegex(commonItems::catchallRegex, [this](const std::string& artifactID, std::istream& stream) {
			const auto token = commonItems::stringOfItem(stream).getString();
			if (token.find('{') == std::string::npos)
				return;
			std::stringstream tokenStream(token);
			ArtifactEntry entry(tokenStream);
			if (entry.artifact.owner <= 0 || entry.artifact.name.empty())
				return;
			try
			{
				entry.artifact.ID = std::stoll(artifactID);
			}
			catch (...)
			{
				return;
			}
			artifacts.push_back(entry.artifact);
		});
		registryParser.parseStream(theStream);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}
