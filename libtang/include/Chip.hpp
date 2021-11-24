#ifndef LIBTANG_CHIP_HPP
#define LIBTANG_CHIP_HPP

#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <map>
#include <set>

using namespace std;
namespace Tang {

// Basic information about a chip that may be needed elsewhere
struct ChipInfo
{
    string name;
    string family;
    uint32_t idcode;
    int num_frames;
    int bits_per_frame;
    int pad_bits_before_frame;
    int pad_bits_after_frame;
    // 0-based.
    int max_row;
    int max_col;
    // Trellis uses 0-based indexing, but some devices don't.
    int col_bias;
};

}

#endif //LIBTANG_CHIP_HPP
