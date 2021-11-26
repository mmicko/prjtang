#ifndef LIBTANG_CHIP_HPP
#define LIBTANG_CHIP_HPP

#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <map>
#include <set>
#include "CRAM.hpp"

using namespace std;
namespace Tang {

// Basic information about a chip that may be needed elsewhere
struct ChipInfo
{
    string name;
    string family;
    uint16_t num_frames;
    uint32_t bits_per_frame;
    uint32_t bram_bits_per_frame;
    int max_row;
    int max_col;
};


class Chip
{
public:
    // Construct a chip by looking up part name
    explicit Chip(string name);

    // Construct a chip by looking up device ID
    explicit Chip(uint32_t idcode);

    // Construct a chip from a ChipInfo
    explicit Chip(const ChipInfo &info);

    // Basic information about a chip
    ChipInfo info;

    // The chip's configuration memory
    CRAM cram;

    // Miscellaneous information
    uint32_t usercode = 0x00000000;
    uint32_t cfg1;
    uint32_t cfg2;
    vector<string> metadata;

    // Block RAM initialisation
    map<uint8_t, vector<uint8_t>> bram_data;

    // Get max row and column
    int get_max_row() const;

    int get_max_col() const;
};

}

#endif //LIBTANG_CHIP_HPP
