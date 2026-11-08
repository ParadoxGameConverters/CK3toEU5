#ifndef OUT_POPS_FILE_H
#define OUT_POPS_FILE_H

#include <filesystem>
#include <string>

#include "src/eu5_world/eu5_location_data.hpp"
#include "src/eu5_world/eu5_world.hpp"
#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

// Rewrites setup/start pops so populations follow the CK3 save's religion.
//
// Only religion is converted. EU5's own pop types, sizes and cultures are kept, because there is no
// CK3 culture to EU5 culture mapping to convert cultures with - the configurables cover culture
// groups by heritage and nothing finer - and EU5's sizes carry its economic balance.
class PopsFile: public OutputFile
{
  public:
   PopsFile(const std::string& name,
       FileWriter& file_writer,
       const eu5::EU5World& eu5_world,
       const eu5::LocationData& location_data);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   const eu5::EU5World& eu5_world_;
   const eu5::LocationData& location_data_;
};

}  // namespace out

#endif  // OUT_POPS_FILE_H
