
#include "landed_titles.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "CommonRegexes.h"
#include "Parser.h"
#include "ParserHelpers.h"
#include "landed_title.hpp"

// This is a class that scrapes 00_landed_titles.txt (and related files) looking for title colors,
// landlessness, and most importantly relation between baronies and barony provinces so we can link titles to actual
// clay. Since titles are nested according to hierarchy we do this recursively.

void ck3::LandedTitles::LoadTitles(const std::filesystem::path& file_name)
{
   commonItems::parser parser;
   parser.registerRegex(R"((h|e|k|d|c|b)_[A-Za-z0-9_\-\']+)",
       [this](const std::string& title_key, std::istream& input_stream) {
          // Start recursion from a top level title, which has no parent.
          ParseLandedTitle(input_stream, title_key, "");
       });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseFile(file_name);
   parser.clearRegisteredKeywords();
}

void ck3::LandedTitles::LoadTitles(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerRegex(R"((h|e|k|d|c|b)_[A-Za-z0-9_\-\']+)",
       [this](const std::string& title_key, std::istream& input_stream) {
          // Start recursion from a top level title, which has no parent.
          ParseLandedTitle(input_stream, title_key, "");
       });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}

void ck3::LandedTitles::ParseLandedTitle(std::istream& input_stream,
    const std::string& title_key,
    const std::string& parent_key)
{
   const std::shared_ptr<LandedTitle> new_title = std::make_shared<LandedTitle>(title_key);
   new_title->SetParentKey(parent_key);
   commonItems::parser parser;
   parser.registerRegex(R"((h|e|k|d|c|b)_[A-Za-z0-9_\-\']+)",
       [this, title_key](const std::string& child_key, std::istream& input_stream) {
          // Parse recursively, remembering what this title sits under.
          ParseLandedTitle(input_stream, child_key, title_key);
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
   parser.registerKeyword("ruler_uses_title_name", [this, new_title](const std::string&, std::istream& input_stream) {
      new_title->SetRulerUsesTitleName(commonItems::singleString(input_stream).getString() == "yes");
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
   landed_titles_[title_key] = new_title;  // override in case of modded titles
   if (!parent_key.empty())
   {
      auto& siblings = children_[parent_key];
      if (std::ranges::find(siblings, title_key) == siblings.end())
      {
         siblings.emplace_back(title_key);
      }
   }
}

const std::vector<std::string>& ck3::LandedTitles::GetChildren(const std::string& title_key) const
{
   static const std::vector<std::string> kNoChildren;
   const auto children = children_.find(title_key);
   return children == children_.end() ? kNoChildren : children->second;
}
