#include "file_writer_impl.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <print>  // NOLINT: couldn't get github runners to use newest c++ standard, should be fixed in the future
#include <stdexcept>
#include <string>



namespace out
{

void FileWriterImpl::CreateEmptyAndWrite(const std::filesystem::path& file_path, const std::string& content)
{
   if (std::filesystem::exists(file_path))
   {
      throw std::runtime_error(
          std::format("Could not create file {}, attempting to create same file twice!", file_path.string()));
   }

   std::ofstream write_file(file_path);
   if (!write_file.is_open())
   {
      throw std::runtime_error(std::format("Could not create file {}", file_path.string()));
   }
   std::print(  // NOLINT: couldn't get github runners to use newest c++ standard, should be fixed in the future
       write_file,
       "{}",
       content);
   write_file.close();
}

}  // namespace out