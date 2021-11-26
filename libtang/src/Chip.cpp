#include "Chip.hpp"
#include "Tile.hpp"
#include "Database.hpp"
#include "Util.hpp"
#include <algorithm>
#include <iostream>
using namespace std;

namespace Tang {

Chip::Chip(string name) : Chip(get_chip_info(find_device_by_name(name)))
{}

Chip::Chip(uint32_t idcode) : Chip(get_chip_info(find_device_by_idcode(idcode)))
{}

Chip::Chip(const Tang::ChipInfo &info) : info(info), cram(info.num_frames, info.bits_per_frame)
{}

int Chip::get_max_row() const
{
    return info.max_row;
}

int Chip::get_max_col() const
{
    return info.max_col;
}

}
