#ifndef OUT_WORLD_H
#define OUT_WORLD_H
#include <filesystem>
#include <string>

namespace configuration
{
class Configuration;
}

namespace EU5
{
class World;

// Writes the converted EU5 world as a loadable EU5 mod, staged in output/ for Fronter to install.
void outputWorld(const World& world, const configuration::Configuration& theConfiguration);
} // namespace EU5

#endif // OUT_WORLD_H
