#include "save_melter.hpp"

#include <external/rakaly/rakaly.h>

#include <filesystem>
#include <fstream>

#include "Log.h"

void ck3::SaveMelter::VerifySave(const std::filesystem::path& save_game_path)
{
   std::ifstream save_file(save_game_path, std::ios::binary);
   if (!save_file.is_open())
   {
      throw std::runtime_error("Could not open save! Exiting!");
   }

   char buffer[10];  // NOLINT : multiple rules dislike c arrays, maybe to be fixed in future
   save_file.get(static_cast<char*>(buffer), 4);
   if (buffer[0] != 'S' || buffer[1] != 'A' || buffer[2] != 'V')
   {
      throw std::runtime_error("Savefile of unknown type.");
   }

   save_file.close();
}

ck3::SaveData ck3::SaveMelter::MeltSave(const std::filesystem::path& save_game_path, bool debug)
{
   const std::ifstream save_file(save_game_path, std::ios::binary);
   std::stringstream in_stream;
   in_stream << save_file.rdbuf();
   std::string save_game_string = in_stream.str();
   SaveData save_game;

   const auto save = rakaly::parseCk3(save_game_string);

   if (const auto& melt = save.meltMeta(); melt)
   {
      Log(LogLevel::Info) << "Meta extracted successfully.";
      melt->writeData(save_game.metadata);
   }
   else
   {
      Log(LogLevel::Warning) << "NO META!";
      save_game.metadata = "none";
      if (save.is_binary())
      {
         Log(LogLevel::Error) << "Binary Save and NO META!";
      }
   }

   if (save.is_binary())
   {
      Log(LogLevel::Info) << "Gamestate is binary";
   }

   const auto& melt = save.melt();
   melt.writeData(save_game.gamestate);
   if (melt.has_unknown_tokens())
   {
      Log(LogLevel::Error) << "Rakaly reports errors while melting save!";
   }

   if (debug)
   {
      Log(LogLevel::Info) << "Debug is active: Dumping metadata and gamestate to txt files.";
      std::ofstream meta_dump("metaDump.txt");
      meta_dump << save_game.metadata;
      meta_dump.close();

      std::ofstream save_dump("saveDump.txt");
      save_dump << save_game.gamestate;
      save_dump.close();
   }

   return save_game;
}