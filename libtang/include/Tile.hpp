#ifndef LIBTANG_TILE_HPP
#define LIBTANG_TILE_HPP

#include <string>
#include <iostream>
#include <cstdint>
#include <utility>
#include <regex>
#include <cassert>
#include <map>
#include "CRAM.hpp"

namespace Tang {

// Basic information about a tile
struct TileInfo {
    string family;
    string device;
    size_t max_col;
    size_t max_row;

    string name;
    string type;
    size_t num_frames;
    size_t bits_per_frame;
    size_t frame_offset;
    size_t bit_offset;
    size_t row;
    size_t col;

    inline pair<int, int> get_row_col() const {
        return make_pair((int)row,(int)col);
    };

};


class Chip;
// Represents an actual tile
class Tile {
public:
    Tile(TileInfo info, Chip &parent);
    TileInfo info;
    CRAMView cram;

    // Dump the tile textual config as a string
    string dump_config() const;
    // Configure the tile from a string config
    void read_config(string config);
    // Set by dump_config
    mutable int known_bits = 0;
    mutable int unknown_bits = 0;
};

}


#endif //LIBTANG_TILE_HPP
