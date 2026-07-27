#include "NamedColors.h"
#include "CommonRegexes.h"
#include "Log.h"
#include "ParserHelpers.h"

commonItems::Color::Factory laFabricaDeColor;

void mappers::NamedColors::loadColors(std::istream& theStream)
{
	registerKeys();
	parseStream(theStream);
	clearRegisteredKeywords();
}

void mappers::NamedColors::loadColors(const std::filesystem::path& filepath)
{
	registerKeys();
	parseFile(filepath);
	clearRegisteredKeywords();
}

void mappers::NamedColors::registerKeys()
{
	// If we get a color named "colors", we're f--d.
	registerKeyword("colors", [](const std::string&, std::istream& theStream) {
		auto loadedColors = NamedColors();
		loadedColors.loadColors(theStream);
	});
	registerRegex(R"(khitan|tungusic|0.00)", [](const std::string&, std::istream&) {
		// Ignoring very specific junk.
	});
	registerRegex(commonItems::catchallRegex, [](const std::string& colorName, std::istream& theStream) {
		try
		{
			laFabricaDeColor.addNamedColor(colorName, theStream);
		}
		catch (std::exception& e)
		{
			Log(LogLevel::Warning) << "Issue parsing named colors: " << colorName << " : " << e.what();
		}
	});
}