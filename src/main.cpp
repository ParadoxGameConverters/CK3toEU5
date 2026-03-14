#include <external/commonItems/Log.h>

#include <exception>
#include <print>

#include "ck3_to_eu5_converter.hpp"



int main()  // NOLINT(bugprone-exception-escape)
{
   try
   {
      Log(LogLevel::Progress) << "0%";

      // Meet C++23's new hotness!
      std::println("Hello, world!");
      ck3_to_eu5::ConvertCk3ToEu5();
   }
   catch (const std::exception& e)
   {
      Log(LogLevel::Error) << e.what();
      return -1;
   }

   return 0;
}