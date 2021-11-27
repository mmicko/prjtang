#ifndef LIBTANG_CHIPCONFIG_HPP
#define LIBTANG_CHIPCONFIG_HPP

#include "TileConfig.hpp"
#include <map>
#include <vector>
#include <string>

using namespace std;

namespace Tang {

class Chip;

// A group of tiles to configure at once for a particular feature that is split across tiles
// TileGroups are currently for non-routing configuration only
struct TileGroup
{
    vector<string> tiles;
    TileConfig config;
};

// This represents the configuration of a chip at a high level
class ChipConfig
{
public:
    string chip_name;
    vector<string> metadata;
    map<string, TileConfig> tiles;
    vector<TileGroup> tilegroups;
    map<string, string> sysconfig;

    // Block RAM initialisation
    map<uint8_t, vector<uint8_t>> bram_data;

    string to_string() const;
    static ChipConfig from_string(const string &config);
    Chip to_chip() const;
    static ChipConfig from_chip(const Chip &chip);
};

}

#endif // LIBTANG_CHIPCONFIG_HPP
