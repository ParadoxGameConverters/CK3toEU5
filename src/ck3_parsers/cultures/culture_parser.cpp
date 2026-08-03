#include "culture_parser.hpp"

#include <format>
#include <iostream>
#include <sstream>

#include "external/commonItems/CommonRegexes.h"
#include "external/commonItems/Log.h"
#include "external/commonItems/Parser.h"
#include "external/commonItems/ParserHelpers.h"

namespace
{

const int kNameListPrefixLength = 10;

}  // namespace

ck3::CultureParser::CultureParser(std::istream& input_stream, long long culture_id): culture_id_(culture_id)
{
   ParseCulture(input_stream);

   // Resolve a working name. Vanilla cultures have a culture_template (e.g. "czech").
   // Hybrid/divergent cultures only have a localized name the player (or game) gave them.
   if (culture_template_.has_value())
   {
      name_ = *culture_template_;
   }
   else
   {
      dynamic_ = true;
      if (localized_name_.has_value())
      {
         name_ = *localized_name_;
      }
      else
      {
         Log(LogLevel::Warning) << std::format("Failed to resolve a name for culture {}", culture_id_);
         name_ = "noname";
      }
   }
}

void ck3::CultureParser::ParseCulture(std::istream& input_stream)
{
   registerKeyword("culture_template", [this](std::istream& input_stream) {
      culture_template_ = commonItems::getString(input_stream);
   });
   registerKeyword("name", [this](std::istream& input_stream) {
      localized_name_ = commonItems::getString(input_stream);
   });
   registerKeyword("heritage", [this](std::istream& input_stream) {
      heritage_ = commonItems::getString(input_stream);
   });
   registerKeyword("language", [this](std::istream& input_stream) {
      language_ = commonItems::getString(input_stream);
   });
   // registerKeyword("color", [this](std::istream& input_stream) {
   //	color_ = laFabricaDeColor.getColor(input_stream);
   // });
   registerKeyword("ethos", [this](std::istream& input_stream) {
      ethos_ = commonItems::singleString(input_stream).getString();
   });
   registerKeyword("traditions", [this](std::istream& input_stream) {
      traditions_ = commonItems::getStrings(input_stream);
   });
   // Eras are listed oldest-first. The one the culture currently sits in is the last that carries a
   // join date; the very first era needs none, since every culture starts there.
   registerKeyword("culture_era_data", [this](std::istream& input_stream) {
      for (const auto& blob: commonItems::blobList(input_stream).getBlobs())
      {
         auto blob_stream = std::stringstream("{" + blob + "}");
         std::string era_type;
         auto joined = false;
         commonItems::parser era_parser;
         era_parser.registerKeyword("type", [&era_type](std::istream& stream) {
            era_type = commonItems::getString(stream);
         });
         era_parser.registerKeyword("join", [&joined](std::istream& stream) {
            commonItems::ignoreItem("join", stream);
            joined = true;
         });
         era_parser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
         era_parser.parseStream(blob_stream);
         if (!era_type.empty() && (joined || era_.empty()))
         {
            era_ = era_type;
         }
      }
   });
   // registerKeyword("culture_innovation", [this](std::istream& input_stream) {
   //	innovationCount += static_cast<int>(commonItems::blobList(input_stream).getBlobs().size());
   // });
   registerKeyword("name_list", [this](std::istream& input_stream) {
      auto temp = commonItems::getString(input_stream);
      if (temp.size() > kNameListPrefixLength)
      {
         temp = temp.substr(kNameListPrefixLength, temp.size());  // drop "name_list_", leave "polish"
         name_set_.insert(temp);
      }
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

   parseStream(input_stream);
   clearRegisteredKeywords();
}
