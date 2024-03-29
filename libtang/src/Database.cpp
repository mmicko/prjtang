#include "Database.hpp"
#include "Chip.hpp"
#include "Tile.hpp"
#include "Util.hpp"
#include "BitDatabase.hpp"
#include <iostream>
#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <stdexcept>
#include <mutex>



namespace pt = boost::property_tree;

namespace Tang {
static string db_root = "";
static pt::ptree devices_info;

// Cache Tilegrid data, to save time parsing it again
static map<string, pt::ptree> tilegrid_cache;
#ifndef NO_THREADS
static mutex tilegrid_cache_mutex;
#endif

void load_database(string root) {
    db_root = root;
    pt::read_json(root + "/" + "devices.json", devices_info);
}

// Iterate through all family and device permutations
// T should return true in case of a match
template<typename T>
boost::optional<DeviceLocator> find_device_generic(T f) {
    for (const pt::ptree::value_type &family : devices_info.get_child("families")) {
        for (const pt::ptree::value_type &dev : family.second.get_child("devices")) {
            for (const pt::ptree::value_type &pkg : dev.second.get_child("packages")) {
                bool res = f(dev.first, dev.second, pkg.first);
                if (res)
                    return boost::make_optional(DeviceLocator{family.first, dev.first, pkg.first});
            }
        }
    }
    return boost::optional<DeviceLocator>();
}

DeviceLocator find_device_by_name(string name, string package) {
    auto found = find_device_generic([name, package](const string &n, const pt::ptree &p, const string &pk) -> bool {
        UNUSED(p);
        return (n == name) && (pk == package);
    });
    if (!found)
        throw runtime_error("no device in database with name " + name);
    return *found;
}

DeviceLocator find_device_by_part(string part) {
    auto found = find_device_generic([part](const string &n, const pt::ptree &p, const string &pk) -> bool {
        UNUSED(n);
        for (const pt::ptree::value_type &pkg : p.get_child("packages")) {
            if ((pkg.second.get<string>("part") == part) && (pkg.first == pk)) return true;
        }
        return false;
    });
    if (!found)
        throw runtime_error("no device in database with part name " + part);
    return *found;
}

DeviceLocator find_device_by_idcode(uint32_t idcode) {
    auto found = find_device_generic([idcode](const string &n, const pt::ptree &p, const string &pk) -> bool {
        UNUSED(n);
        for (const pt::ptree::value_type &pkg : p.get_child("packages")) {
            if ((parse_uint32(pkg.second.get<string>("idcode")) == idcode) && (pkg.first == pk)) return true;
        }
        return false;
    });
    if (!found)
        throw runtime_error("no device in database with IDCODE " + uint32_to_hexstr(idcode));
    return *found;
}

ChipInfo get_chip_info(const DeviceLocator &part) {
    pt::ptree dev = devices_info.get_child("families").get_child(part.family).get_child("devices").get_child(
            part.device);
    pt::ptree pkg = dev.get_child("packages").get_child(part.package);
    ChipInfo ci;
    ci.family = part.family;
    ci.name = part.device;
    ci.package = part.package;
    ci.idcode = parse_uint32(pkg.get<string>("idcode"));
    ci.num_frames = dev.get<int>("frames");
    ci.bits_per_frame = dev.get<int>("bits_per_frame");
    ci.bram_bits_per_frame = dev.get<int>("bram_bits_per_frame");
    ci.max_row = dev.get<int>("max_row");
    ci.max_col = dev.get<int>("max_col");
    return ci;
}

vector<TileInfo> get_device_tilegrid(const DeviceLocator &part) {
    vector <TileInfo> tilesInfo;
    assert(db_root != "");
    string tilegrid_path = db_root + "/" + part.family + "/" + part.device + "/tilegrid.json";
    {
        ChipInfo info = get_chip_info(part);
#ifndef NO_THREADS
        lock_guard <mutex> lock(tilegrid_cache_mutex);
#endif
        if (tilegrid_cache.find(part.device) == tilegrid_cache.end()) {
            pt::ptree tg_parsed;
            pt::read_json(tilegrid_path, tg_parsed);
            tilegrid_cache[part.device] = tg_parsed;
        }
        const pt::ptree &tg = tilegrid_cache[part.device];

        for (const pt::ptree::value_type &tile : tg) {
            TileInfo ti;
            ti.family = part.family;
            ti.device = part.device;
            ti.max_col = info.max_col;
            ti.max_row = info.max_row;

            ti.name = tile.first;
            ti.col = size_t(tile.second.get<int>("x"));
            ti.row = size_t(tile.second.get<int>("y"));
            ti.num_frames = size_t(tile.second.get<int>("rows"));
            ti.bits_per_frame = size_t(tile.second.get<int>("cols"));
            ti.bit_offset = size_t(tile.second.get<int>("start_bit"));
            // For eagle_s20 only
            if (ti.bit_offset >=974) ti.bit_offset += 6;
			if (ti.bit_offset >=2920+6) ti.bit_offset += 6;
            ti.frame_offset = size_t(tile.second.get<int>("start_frame"));
            ti.type = tile.second.get<string>("type");
            ti.flag = size_t(tile.second.get<int>("flag"));
            tilesInfo.push_back(ti);
        }
    }
    return tilesInfo;
}


static unordered_map<TileLocator, shared_ptr<TileBitDatabase>> bitdb_store;
#ifndef NO_THREADS
static mutex bitdb_store_mutex;
#endif

shared_ptr<TileBitDatabase> get_tile_bitdata(const TileLocator &tile) {
#ifndef NO_THREADS
    lock_guard <mutex> bitdb_store_lg(bitdb_store_mutex);
#endif
    if (bitdb_store.find(tile) == bitdb_store.end()) {
        assert(!db_root.empty());
        string bitdb_path = db_root + "/" + tile.family + "/tiledata/" + tile.tiletype + "/bits.db";
        shared_ptr <TileBitDatabase> bitdb{new TileBitDatabase(bitdb_path)};
        bitdb_store[tile] = bitdb;
        return bitdb;
    } else {
        return bitdb_store.at(tile);
    }
}


}
