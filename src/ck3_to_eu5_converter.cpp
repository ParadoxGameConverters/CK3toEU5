#include "ck3_to_eu5_converter.hpp"

#include <external/commonItems/Log.h>

#include "src/ck3_world/Characters/Character.h"
#include "src/ck3_world/Titles/Title.h"
#include "src/ck3_world/World.h"
#include "src/eu5_world/World.h"
#include "src/output/outWorld.h"

namespace ck3_to_eu5
{

void ConvertCk3ToEu5(const configuration::Configuration& configuration, const commonItems::ConverterVersion& converter_version)
{
   const CK3::World ck3_world(configuration, converter_version);

   Log(LogLevel::Info) << "*** CK3 World Summary ***";
   Log(LogLevel::Info) << "Conversion date: " << ck3_world.getConversionDate();
   Log(LogLevel::Info) << "Independent realms: " << ck3_world.getIndeps().size();
   if (const auto& player_title = ck3_world.getPlayerTitle(); player_title)
   {
      Log(LogLevel::Info) << "Player realm: " << *player_title;
   }

   const EU5::World eu5_world(ck3_world, configuration);
   EU5::outputWorld(eu5_world, configuration);

   Log(LogLevel::Progress) << "100%";
   Log(LogLevel::Notice) << "* Conversion complete *";
}

}  // namespace ck3_to_eu5
