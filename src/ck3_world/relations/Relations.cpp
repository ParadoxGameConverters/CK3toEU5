#include "Relations.h"

#include <sstream>

#include "CommonRegexes.h"
#include "ParserHelpers.h"

namespace
{
class RelationEntry: commonItems::parser
{
  public:
   explicit RelationEntry(std::istream& theStream)
   {
      registerKeyword("first", [this](std::istream& stream) {
         first = commonItems::getLlong(stream);
      });
      registerKeyword("second", [this](std::istream& stream) {
         second = commonItems::getLlong(stream);
      });
      registerKeyword("alliances", [this](std::istream& stream) {
         commonItems::ignoreItem("alliances", stream);
         allied = true;
      });
      registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
      parseStream(theStream);
      clearRegisteredKeywords();
   }

   long long first = 0;
   long long second = 0;
   bool allied = false;
};
}  // namespace

CK3::Relations::Relations(std::istream& theStream)
{
   registerKeys();
   parseStream(theStream);
   clearRegisteredKeywords();
}

void CK3::Relations::registerKeys()
{
   registerKeyword("active_relations", [this](std::istream& theStream) {
      // The list is a series of anonymous { first=X second=Y ... } blocks.
      const auto blobs = commonItems::blobList(theStream).getBlobs();
      for (const auto& blob: blobs)
      {
         std::stringstream blobStream(blob);
         const RelationEntry entry(blobStream);
         if (entry.allied && entry.first > 0 && entry.second > 0)
            alliancePairs.emplace_back(entry.first, entry.second);
      }
   });
   registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);
}
