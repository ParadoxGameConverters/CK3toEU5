#ifndef OUT_CULTURE_DEFINITIONS_FILE_H
#define OUT_CULTURE_DEFINITIONS_FILE_H

#include <filesystem>
#include <string>

#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Writes common/cultures definitions for CK3 cultures EU5 has no equivalent for.
//
// EU5 ships no norse culture, and no campaign's hybrid or divergent cultures exist in it at all.
// Forcing those into the nearest EU5 culture would throw away exactly the part of a CK3 game worth
// carrying over, so they are defined instead, taking their language and groups from the
// configurables and keeping their CK3 name.
class CultureDefinitionsFile: public OutputFile
{
  public:
   CultureDefinitionsFile(const std::string& name, FileWriter& file_writer, const eu5::EU5World& eu5_world);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
};

}  // namespace out

#endif  // OUT_CULTURE_DEFINITIONS_FILE_H
