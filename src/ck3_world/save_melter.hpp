#ifndef CK3_SAVE_MELTER_H
#define CK3_SAVE_MELTER_H

#include <filesystem>
#include <string>

namespace ck3
{
struct SaveData
{
   std::string gamestate;
   std::string metadata;  // we use this to set up mods before main processing.
};

class SaveMelter
{
  public:
   static void VerifySave(const std::filesystem::path& save_game_path);
   static SaveData MeltSave(const std::filesystem::path& save_game_path, bool debug);
};
}  // namespace ck3

#endif  // CK3_SAVE_MELTER_H