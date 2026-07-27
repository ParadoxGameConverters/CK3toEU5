#include "Confederations.h"
#include "../CoatsOfArms/CoatsOfArms.h"
#include "CommonRegexes.h"
#include "Confederation.h"
#include "Log.h"
#include "ParserHelpers.h"
#include <ranges>

CK3::Confederations::Confederations(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void CK3::Confederations::registerKeys()
{
	databaseParser.registerRegex(R"(\d+)", [this](const std::string& confederationID, std::istream& theStream) {
		const auto questionableItem = commonItems::stringOfItem(theStream).getString();
		if (questionableItem.find('{') == std::string::npos)
			return; // a disbanded bloc, written as "id=none"
		auto tempStream = std::stringstream(questionableItem);
		try
		{
			auto newConfederation = std::make_shared<Confederation>(tempStream, std::stoll(confederationID));
			confederations.emplace(newConfederation->getID(), newConfederation);
		}
		catch (std::exception&)
		{
			Log(LogLevel::Warning) << "Invalid confederation blob at ID: " << confederationID;
		}
	});
	databaseParser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

	registerKeyword("database", [this](std::istream& theStream) {
		databaseParser.parseStream(theStream);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}

void CK3::Confederations::linkCoats(const CoatsOfArms& coats)
{
	auto counter = 0;
	const auto& coatData = coats.getCoats();
	for (const auto& confederation: confederations | std::views::values)
	{
		if (!confederation->getCoat())
			continue;
		const auto& coatDataItr = coatData.find(confederation->getCoat()->first);
		if (coatDataItr == coatData.end())
		{
			Log(LogLevel::Warning) << "Confederation " << confederation->getName() << " has CoA " << confederation->getCoat()->first
										  << " which has no definition. Defaulting to leading member CoA.";
			continue;
		}
		confederation->loadCoat(*coatDataItr);
		++counter;
	}
	Log(LogLevel::Info) << "<> " << counter << " confederations updated.";
}
