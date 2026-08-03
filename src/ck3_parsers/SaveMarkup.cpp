#include "SaveMarkup.h"

std::string ck3::stripSaveMarkup(const std::string& text)
{
   std::string plain;
   plain.reserve(text.size());
   for (size_t position = 0; position < text.size();)
   {
      // Quotes inside a saved string arrive still escaped; the loc writer escapes them its own way.
      if (text[position] == '\\' && position + 1 < text.size() && text[position + 1] == '"')
      {
         plain += '"';
         position += 2;
         continue;
      }
      if (text[position] != '\x15')
      {
         plain += text[position++];
         continue;
      }
      ++position;
      // 0x15! closes a command; anything else opens one, named up to the next space.
      if (position < text.size() && text[position] == '!')
      {
         ++position;
         continue;
      }
      while (position < text.size() && text[position] != ' ' && text[position] != '\x15')
         ++position;
      if (position < text.size() && text[position] == ' ')
         ++position;  // the space separates the command from its text and isn't part of either
   }

   // CK3 leaves gaps where a referenced thing no longer exists, which reads as stray whitespace -
   // "inlaid with ." and the like.
   const std::string closingPunctuation = ".,;:!?)";
   std::string tidied;
   tidied.reserve(plain.size());
   for (const auto character: plain)
   {
      const auto previous = tidied.empty() ? '\0' : tidied.back();
      if (character == ' ' && (previous == ' ' || previous == '\n' || previous == '\0'))
         continue;
      if (previous == ' ' && closingPunctuation.find(character) != std::string::npos)
         tidied.back() = character;
      else
         tidied += character;
   }
   while (!tidied.empty() && (tidied.back() == ' ' || tidied.back() == '\n'))
      tidied.pop_back();
   return tidied;
}
