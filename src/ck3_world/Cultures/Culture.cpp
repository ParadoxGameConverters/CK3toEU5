#include "Culture.h"
#include "CommonRegexes.h"
#include "Log.h"
#include "ParserHelpers.h"
#include <sstream>

CK3::Culture::Culture(std::istream& theStream, long long theID): ID(theID)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();

	// Resolve a working name. Vanilla cultures have a culture_template (e.g. "czech").
	// Hybrid/divergent cultures only have a localized name the player (or game) gave them.
	if (culture_template)
	{
		name = *culture_template;
	}
	else
	{
		dynamic = true;
		if (localizedName)
			name = *localizedName;
		else
			name = "noname";
	}
}

void CK3::Culture::registerKeys()
{
	registerKeyword("culture_template", [this](std::istream& theStream) {
		culture_template = commonItems::getString(theStream);
	});
	registerKeyword("name", [this](std::istream& theStream) {
		localizedName = commonItems::getString(theStream);
	});
	registerKeyword("heritage", [this](std::istream& theStream) {
		heritage = commonItems::getString(theStream);
	});
	registerKeyword("language", [this](std::istream& theStream) {
		language = commonItems::getString(theStream);
	});
	registerKeyword("color", [this](std::istream& theStream) {
		color = laFabricaDeColor.getColor(theStream);
	});
	registerKeyword("ethos", [this](std::istream& theStream) {
		ethos = commonItems::singleString(theStream).getString();
	});
	registerKeyword("traditions", [this](std::istream& theStream) {
		traditions = commonItems::getStrings(theStream);
	});
	// Eras are listed oldest-first. The one the culture currently sits in is the last that carries a
	// join date; the very first era needs none, since every culture starts there.
	registerKeyword("culture_era_data", [this](std::istream& theStream) {
		for (const auto& blob: commonItems::blobList(theStream).getBlobs())
		{
			auto blobStream = std::stringstream("{" + blob + "}");
			std::string eraType;
			auto joined = false;
			commonItems::parser eraParser;
			eraParser.registerKeyword("type", [&eraType](std::istream& stream) {
				eraType = commonItems::getString(stream);
			});
			eraParser.registerKeyword("join", [&joined](std::istream& stream) {
				commonItems::ignoreItem("join", stream);
				joined = true;
			});
			eraParser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
			eraParser.parseStream(blobStream);
			if (!eraType.empty() && (joined || era.empty()))
				era = eraType;
		}
	});
	registerKeyword("culture_innovation", [this](std::istream& theStream) {
		innovationCount += static_cast<int>(commonItems::blobList(theStream).getBlobs().size());
	});
	registerKeyword("name_list", [this](std::istream& theStream) {
		auto temp = commonItems::getString(theStream);
		if (temp.size() > 10)
		{
			temp = temp.substr(10, temp.size()); // drop "name_list_", leave "polish"
			nameLists.insert(temp);
		}
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}
