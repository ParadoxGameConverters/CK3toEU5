#include "Opinions.h"

#include <sstream>

#include "CommonRegexes.h"
#include "ParserHelpers.h"

namespace
{
class OpinionEntry: commonItems::parser
{
  public:
   explicit OpinionEntry(std::istream& theStream)
   {
      registerKeyword("owner", [this](std::istream& stream) {
         owner = commonItems::getLlong(stream);
      });
      registerKeyword("target", [this](std::istream& stream) {
         target = commonItems::getLlong(stream);
      });
      registerKeyword("scripted_relations", [this](std::istream& stream) {
         commonItems::parser relationsParser;
         relationsParser.registerKeyword("rival", [this](std::istream& rivalStream) {
            commonItems::ignoreItem("rival", rivalStream);
            rival = true;
         });
         relationsParser.registerKeyword("nemesis", [this](std::istream& nemesisStream) {
            commonItems::ignoreItem("nemesis", nemesisStream);
            rival = true;
         });
         relationsParser.registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
         relationsParser.parseStream(stream);
      });
      registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      parseStream(theStream);
      clearRegisteredKeywords();
   }

   long long owner = 0;
   long long target = 0;
   bool rival = false;
};
}  // namespace

CK3::Opinions::Opinions(std::istream& theStream)
{
   registerKeys();
   parseStream(theStream);
   clearRegisteredKeywords();
}

void CK3::Opinions::registerKeys()
{
   registerKeyword("active_opinions", [this](std::istream& theStream) {
      for (const auto& blob: commonItems::blobList(theStream).getBlobs())
      {
         std::stringstream blobStream(blob);
         const OpinionEntry entry(blobStream);
         if (entry.rival && entry.owner > 0 && entry.target > 0)
            rivalPairs.emplace_back(entry.owner, entry.target);
      }
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}
