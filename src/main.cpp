#include <print>

#include "ck3_to_eu5_converter.hpp"



int main()
{
   try
   {
      // I miss printf()
      // Meet C++23's new hotness!

      std::println("Hello, world!");
      ck3_to_eu5::ConvertCk3ToEu5();
   }
   catch (...)
   {
      std::println("An error occurred.");
   }

   return 0;
}