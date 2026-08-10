#ifndef CK3_SAVE_MARKUP_H
#define CK3_SAVE_MARKUP_H
#include <string>

namespace ck3
{
// CK3 embeds link and formatting commands in saved strings (artifact names, war names...), each
// introduced by 0x15 and closed by 0x15!, wrapping the text the player actually sees. Drops the
// commands and keeps the text.
std::string stripSaveMarkup(const std::string& text);
}  // namespace ck3

#endif  // CK3_SAVE_MARKUP_H
