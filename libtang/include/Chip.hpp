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
    //uint32_t idcode;
    int num_frames;
    int bits_per_frame;
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

    // Get max row and column
    int get_max_row() const;

    int get_max_col() const;
};

}

#endif //LIBTANG_CHIP_HPP
