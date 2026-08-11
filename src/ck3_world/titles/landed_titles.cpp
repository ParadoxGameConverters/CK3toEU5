
#include "landed_titles.hpp"

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "landed_title.hpp"

// This is a recursive class that scrapes 00_landed_titles.txt (and related files) looking for title colors,
// landlessness, and most importantly relation between baronies and barony provinces so we can link titles to actual
// clay. Since titles are nested according to hierarchy we do this recursively.

void ck3::LandedTitles::LoadTitles(const std::filesystem::path& file_name)
{
   commonItems::parser parser;
   parser.registerRegex(R"((h|e|k|d|c|b)_[A-Za-z0-9_\-\']+)",
       [this](const std::string& title_key, std::istream& input_stream) {
          // Start recursion
          ParseLandedTitle(input_stream, title_key);
       });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseFile(file_name);
   parser.clearRegisteredKeywords();
}

void ck3::LandedTitles::ParseLandedTitle(std::istream& input_stream, const std::string& title_key)
{
   const std::shared_ptr<LandedTitle> new_title = std::make_shared<LandedTitle>(title_key);
   commonItems::parser parser;
   parser.registerRegex(R"((h|e|k|d|c|b)_[A-Za-z0-9_\-\']+)",
       [this, new_title](const std::string& title_key, std::istream& input_stream) {
          // Parse recursevily
          ParseLandedTitle(input_stream, title_key);
       });
   parser.registerKeyword("definite_form", [this, new_title](const std::string&, std::istream& input_stream) {
      new_title->SetDefiniteForm(commonItems::singleString(input_stream).getString() == "yes");
   });
   parser.registerKeyword("landless", [this, new_title](const std::string&, std::istream& input_stream) {
      new_title->SetLandless(commonItems::singleString(input_stream).getString() == "yes");
   });
   parser.registerKeyword("province", [this, new_title](const std::string&, std::istream& input_stream) {
      new_title->SetProvince(commonItems::singleInt(input_stream).getInt());
   });
   parser.registerKeyword("can_be_named_after_dynasty",
       [this, new_title](const std::string&, std::istream& input_stream) {
          new_title->SetCanBeNamedAfterDynasty(commonItems::singleString(input_stream).getString() == "yes");
       });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
   landed_titles_.insert(std::make_pair(title_key, new_title));
}
