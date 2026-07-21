#ifndef OUT_RESOURCE_COPY_H
#define OUT_RESOURCE_COPY_H


#include "src/output/out_file_classes/output_file.hpp"

namespace out
{

class CopyResource: public OutputFileOrResource
{
  public:
   CopyResource(std::string name, std::filesystem::path resource_path);

   void Create(const std::filesystem::path& folder_path) override;

  private:
   std::filesystem::path resource_path_;
};

}  // namespace out

#endif  // OUT_OUTPUT_WRITER__H