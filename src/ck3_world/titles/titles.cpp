#include "titles.hpp"

#include <memory>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <exception>
#include <stdexcept>

#include "CommonRegexes.h"
#include "Log.h"
#include "ParserHelpers.h"
#include "Parser.h"
#include "dynamic_title_info.hpp"
#include "title.hpp"


ck3::Titles::Titles(std::istream& input_stream)
{
   ParseTitles(input_stream);


   // This bit assigns CK3::Level to dynamic titles that have a rank definition in the save. It should be all of them,
   // but it's CK3, so who knows.
   if (!dynamic_titles_ranks_.empty())
   {
      TranscribeDynamicTitleRanks();
   }
}

void ck3::Titles::ParseTitles(std::istream& input_stream)
{
   commonItems::parser parser;
   parser.registerKeyword("dynamic_templates", [this](const std::string&, std::istream& input_stream) {
      const commonItems::blobList dynamic_ranks(input_stream);
      for (const auto& dynamic_title: dynamic_ranks.getBlobs())
      {
         std::stringstream blob_stream(dynamic_title);
         const DynamicTitleInfo dynamic_title_info(blob_stream);
         if (!dynamic_title_info.GetDynamicTitleKey().empty() && !dynamic_title_info.GetDynamicTitleRank().empty())
         {
            dynamic_titles_ranks_.insert(
                std::pair(dynamic_title_info.GetDynamicTitleKey(), dynamic_title_info.GetDynamicTitleRank()));
         }
      }
   });
   parser.registerKeyword("landed_titles", [this](const std::string&, std::istream& input_stream) {
      ParseLandedTitles(input_stream);
   });
   parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   parser.parseStream(input_stream);
   parser.clearRegisteredKeywords();
}


void ck3::Titles::ParseLandedTitles(std::istream& input_stream)
{
   commonItems::parser titles_parser;
   titles_parser.registerRegex(R"(\d+)", [this](const std::string& title_id, std::istream& input_stream) {
      // Incoming titles may not be actual titles but half-deleted junk.
      const auto& title_blob = commonItems::stringOfItem(input_stream).getString();

      if (title_blob.contains('{'))
      {
         std::stringstream title_stream(title_blob);
         try
         {
            const std::shared_ptr<Title> new_title = std::make_shared<Title>(title_stream, std::stoll(title_id));
            if (new_title->DoesTitleExist())  // we skip not created/held titles, eu5 doesn't care about them
            {
               auto title_pair = std::pair(new_title->GetKey(), new_title);
               all_titles_.insert(title_pair);
               InsertToCorrectMap(new_title);
            }
         }
         catch (std::exception& e)
         {
            throw std::runtime_error("Cannot import title ID: " + title_id + " (" + e.what() + ")");
         }
      }
   });
   titles_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
   titles_parser.parseStream(input_stream);
   titles_parser.clearRegisteredKeywords();
}

void ck3::Titles::InsertToCorrectMap(const std::shared_ptr<Title>& new_title)
{
   auto title_pair = std::pair(new_title->GetKey(), new_title);
   if (!new_title->IsLandless())
   {
      if (new_title->GetLevel() == Level::kBarony)
      {
         baronies_.insert(title_pair);
      }
      else if (new_title->GetLevel() == Level::kCounty)
      {
         counties_.insert(title_pair);
      }
      else if (new_title->GetLevel() == Level::kDuchy)
      {
         duchies_.insert(title_pair);
      }
      else if (new_title->GetLevel() == Level::kKingdom)
      {
         kingdoms_.insert(title_pair);
      }
      else if (new_title->GetLevel() == Level::kEmpire)
      {
         empires_.insert(title_pair);
      }
      else if (new_title->GetLevel() == Level::kHegemony)
      {
         hegemonies_.insert(title_pair);
      }
   }
}

void ck3::Titles::TranscribeDynamicTitleRanks()
{
   Log(LogLevel::Info) << "-> Transcribing dynamic ranks.";
   auto counter = 0;
   for (const auto& [key, rank]: dynamic_titles_ranks_)
   {
      if (!all_titles_.contains(key))
      {
         continue;  // Probably a destroyed title
      }
      const std::shared_ptr<Title> dynamic_title = all_titles_.at(key);
      auto title_pair = std::pair(key, dynamic_title);
      if (rank == "barony")
      {
         dynamic_title->SetLevel(Level::kBarony);
      }
      else if (rank == "county")
      {
         dynamic_title->SetLevel(Level::kCounty);
      }
      else if (rank == "duchy")
      {
         dynamic_title->SetLevel(Level::kDuchy);
      }
      else if (rank == "kingdom")
      {
         dynamic_title->SetLevel(Level::kKingdom);
      }
      else if (rank == "empire")
      {
         dynamic_title->SetLevel(Level::kEmpire);
      }
      else if (rank == "hegemony")
      {
         dynamic_title->SetLevel(Level::kHegemony);
      }
      InsertToCorrectMap(dynamic_title);
      counter++;
   }
   Log(LogLevel::Info) << "<> Transcribed " << counter << " dynamics.";
}