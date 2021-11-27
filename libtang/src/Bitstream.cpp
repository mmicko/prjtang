#include "Bitstream.hpp"
#include "Chip.hpp"
#include "Util.hpp"
#include <bitset>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/optional.hpp>
#include <cstring>
#include <iostream>

namespace Tang {

static const uint16_t CRC16_POLY = 0x8005;
static const uint16_t CRC16_INIT = 0x0000;

enum class BitstreamCommand : uint8_t {
    DEVICEID = 0xf0,
    VERSION_UCODE =  0xc1,
    CFG_1 = 0xc2,
    CFG_2 = 0xc3,
    FRAMES = 0xc7,
    MEM_FRAME = 0xc8,
    RESET_CRC = 0xf1,
    PROGRAM_DONE = 0xf7,
    CMD_F3 = 0xf3,
    CMD_F5 = 0xf5,
    CMD_C4 = 0xc4,
    CMD_C5 = 0xc5,
    CMD_CA = 0xca,
    FUSE_DATA = 0xec,
    MEMORY_DATA = 0xed,
    //CPLD only
    DEVICEID_CPLD = 0x90,
    RESET_CRC_CPLD = 0xa8,
    CMD_A1 = 0xa1,
    CMD_A3 = 0xa3,
    CMD_AC = 0xac,
    CMD_B1 = 0xb1,
    CPLD_DATA = 0xaa,
    DUMMY_FF = 0xff,
    DUMMY_EE = 0xee,
    DUMMY_00 = 0x00
};

class Crc16 {
public:
    uint16_t crc16 = CRC16_INIT;

    // Add a single byte to the running CRC16 accumulator
    void update_crc16(uint8_t val) {
        int bit_flag;
        for (int i = 7; i >= 0; i--) {
            bit_flag = crc16 >> 15;

            /* Get next bit: */
            crc16 <<= 1;
            crc16 |= (val >> i) & 1; // item a) work from the least significant bits

            /* Cycle check: */
            if (bit_flag)
                crc16 ^= CRC16_POLY;
        }

    }

    uint16_t finalise_crc16() {
        // item b) "push out" the last 16 bits
        int i;
        bool bit_flag;
        for (i = 0; i < 16; ++i) {
            bit_flag = bool(crc16 >> 15);
            crc16 <<= 1;
            if (bit_flag)
                crc16 ^= CRC16_POLY;
        }

        return crc16;
    }

    void reset_crc16() {
        crc16 = CRC16_INIT;
    }
};

// The BitstreamReadWriter class stores state (including CRC16) whilst reading
// the bitstream
class BitstreamReadWriter {
public:
    BitstreamReadWriter() : data(), iter(data.begin()) {};

    BitstreamReadWriter(const vector<uint8_t> &data) : data(data), iter(this->data.begin()) {};

    vector<uint8_t> data;
    vector<uint8_t>::iterator iter;
    Crc16 crc16;

    // Return a single byte and update CRC
    inline uint8_t get_byte() {
        assert(iter < data.end());
        uint8_t val = *(iter++);
        //cerr << hex << setw(2) << int(val) << endl;
        crc16.update_crc16(val);
        return val;
    }

    inline uint16_t get_block_size() {
        uint16_t len = get_uint16();
        //if ((len & 7) != 0)
        //    throw BitstreamParseError("Invalid size value in bitstream");
        return len >> 3;
    }

    // The command opcode is a byte so this works like get_byte
    inline BitstreamCommand get_command_opcode() {
        assert(iter < data.end());
        uint8_t val = *(iter++);
        BitstreamCommand cmd = BitstreamCommand(val);
        crc16.update_crc16(val);
        return cmd;
    }

    // Write a single byte and update CRC
    inline void write_byte(uint8_t b) {
        data.push_back(b);
        crc16.update_crc16(b);
    }

    // Copy multiple bytes into an OutputIterator and update CRC
    template<typename T>
    void get_bytes(T out, size_t count) {
        for (size_t i = 0; i < count; i++) {
            *out = get_byte();
            ++out;
        }
    }

    // Decode a onehot byte, -1 if not onehot
    int decode_onehot(uint8_t in) {
        switch(in) {
            case 0b00000001:
                return 0;
            case 0b00000010:
                return 1;
            case 0b00000100:
                return 2;
            case 0b00001000:
                return 3;
            case 0b00010000:
                return 4;
            case 0b00100000:
                return 5;
            case 0b01000000:
                return 6;
            case 0b10000000:
                return 7;
            default:
                return -1;
        }
    }

    // Write multiple bytes from an InputIterator and update CRC
    template<typename T>
    void write_bytes(T in, size_t count) {
        for (size_t i = 0; i < count; i++)
            write_byte(*(in++));
    }

    // Skip over bytes while updating CRC
    void skip_bytes(size_t count) {
        for (size_t i = 0; i < count; i++) get_byte();
    }

    // Insert zeros while updating CRC
    void insert_zeros(size_t count) {
        for (size_t i = 0; i < count; i++) write_byte(0x00);
    }

    // Insert dummy bytes into the bitstream, without updating CRC
    void insert_dummy(size_t count) {
        for (size_t i = 0; i < count; i++)
            data.push_back(0xFF);
    }

    // Read a big endian uint16 from the bitstream
    uint16_t get_uint16() {
        uint8_t tmp[2];
        get_bytes(tmp, 2);
        return (tmp[0] << 8UL) | (tmp[1]);
    }

    // Read a big endian uint32 from the bitstream
    uint32_t get_uint32() {
        uint8_t tmp[4];
        get_bytes(tmp, 4);
        return (tmp[0] << 24UL) | (tmp[1] << 16UL) | (tmp[2] << 8UL) | (tmp[3]);
    }

    // Write a big endian uint32_t into the bitstream
    void write_uint32(uint32_t val) {
        write_byte(uint8_t((val >> 24UL) & 0xFF));
        write_byte(uint8_t((val >> 16UL) & 0xFF));
        write_byte(uint8_t((val >> 8UL) & 0xFF));
        write_byte(uint8_t(val & 0xFF));
    }

    // Search for a preamble, setting bitstream position to be after the preamble
    // Returns true on success, false on failure
    bool find_preamble(const vector<uint8_t> &preamble) {
        auto found = search(iter, data.end(), preamble.begin(), preamble.end());
        if (found == data.end())
            return false;
        iter = found + preamble.size();
        return true;
    }

    // Get the offset into the bitstream
    size_t get_offset() {
        return size_t(distance(data.begin(), iter));
    }

    // Check the calculated CRC16 against an actual CRC16, expected in the next 2 bytes
    void check_crc16() {
        uint8_t crc_bytes[2];
        uint16_t actual_crc = crc16.finalise_crc16();
        get_bytes(crc_bytes, 2);
        // cerr << hex << int(crc_bytes[0]) << " " << int(crc_bytes[1]) << endl;
        uint16_t exp_crc = (crc_bytes[0] << 8) | crc_bytes[1];
        if (actual_crc != exp_crc) {
            ostringstream err;
            err << "crc fail, calculated 0x" << hex << actual_crc << " but expecting 0x" << exp_crc;
            throw BitstreamParseError(err.str(), get_offset());
        }
        crc16.reset_crc16();
    }

    // Insert the calculated CRC16 into the bitstream, and then reset it
    void insert_crc16() {
        uint16_t actual_crc = crc16.finalise_crc16();
        write_byte(uint8_t((actual_crc >> 8) & 0xFF));
        write_byte(uint8_t((actual_crc) & 0xFF));
        crc16.reset_crc16();
    }

    bool is_end() {
        return (iter >= data.end());
    }

    const vector<uint8_t> &get() {
        return data;
    };
};

Bitstream::Bitstream(const std::vector<uint8_t> &data, const std::vector<std::string> &metadata)
        : data(data), metadata(metadata)
{
}

Bitstream Bitstream::read(std::istream &in)
{
    std::vector<uint8_t> bytes;
    std::vector<std::string> meta;
    auto hdr1 = uint8_t(in.get());
    auto hdr2 = uint8_t(in.get());
    if (hdr1 != 0x23 || hdr2 != 0x20) {
        throw BitstreamParseError("Anlogic .BIT files must start with comment", 0);
    }
    std::string temp;
    uint8_t c;
    in.seekg(0, in.beg);
    while ((c = uint8_t(in.get())) != 0x00) {
        if (in.eof())
            throw BitstreamParseError("Encountered end of file before start of bitstream data");
        if (c == '\n') {
            meta.push_back(temp);
            temp = "";
        } else {
            temp += char(c);
        }
    }
    size_t start_pos = size_t(in.tellg()) - 1;
    in.seekg(0, in.end);
    size_t length = size_t(in.tellg()) - start_pos;
    in.seekg(start_pos, in.beg);
    bytes.resize(length);
    in.read(reinterpret_cast<char *>(&(bytes[0])), length);
    return Bitstream(bytes, meta);
}

// TODO: replace these macros with something more flexible
#define BITSTREAM_DEBUG(x) if (verbosity >= VerbosityLevel::DEBUG) cerr << "bitstream: " << x << endl
#define BITSTREAM_NOTE(x) if (verbosity >= VerbosityLevel::NOTE) cerr << "bitstream: " << x << endl
#define BITSTREAM_FATAL(x, pos) { ostringstream ss; ss << x; throw BitstreamParseError(ss.str(), pos); }

static const vector<uint8_t> preamble = {0xCC, 0x55, 0xAA, 0x33};

Chip Bitstream::deserialise_chip()
{
    BitstreamReadWriter rd(data);
    boost::optional<Chip> chip;

    Crc16 data_crc16;

    bool found_preamble = rd.find_preamble(preamble);

    if (!found_preamble)
        throw BitstreamParseError("preamble not found in bitstream");

    bool is_cpld = false;
    while (!rd.is_end()) {
        uint16_t block_size = rd.get_block_size();
        rd.crc16.reset_crc16();
        BitstreamCommand cmd = rd.get_command_opcode();

        bool is_cpld_command = false;
        switch(cmd) {
            case BitstreamCommand::DUMMY_FF:
            case BitstreamCommand::DUMMY_EE:
            case BitstreamCommand::DUMMY_00:
                // first byte is read as cmd
                BITSTREAM_NOTE("padding block_size " << dec << block_size);
                rd.skip_bytes(block_size-1);
                continue;
                break;

            case BitstreamCommand::DEVICEID_CPLD:
            case BitstreamCommand::RESET_CRC_CPLD:
            case BitstreamCommand::CMD_A1:
            case BitstreamCommand::CMD_A3:
            case BitstreamCommand::CMD_AC:
            case BitstreamCommand::CMD_B1:
            case BitstreamCommand::CPLD_DATA:
                is_cpld_command = true;
                break;
            case BitstreamCommand::CMD_C4:
                if (is_cpld)
                    is_cpld_command = true;
                break;
            default:
                is_cpld_command = false;
        }
        uint16_t cmd_size = 0;
        if (cmd!=BitstreamCommand::MEMORY_DATA) {
            uint8_t flag = rd.get_byte();
            if (!flag) {
                cmd_size = rd.get_uint16();
                if (!is_cpld_command && (block_size - 4) != cmd_size)
                    throw BitstreamParseError("error parsing command");
            }
        }

        switch (cmd) {
            case BitstreamCommand::RESET_CRC:
                BITSTREAM_DEBUG("reset crc");
                rd.get_uint16();
                data_crc16.reset_crc16();
                break;
            case BitstreamCommand::DEVICEID: {
                uint32_t id = rd.get_uint32();
                BITSTREAM_NOTE("device ID: 0x" << hex << setw(8) << setfill('0') << id);
                chip = boost::make_optional(Chip(id));
                chip->metadata = metadata;
                break;
            }
            case BitstreamCommand::VERSION_UCODE:
                chip->usercode = rd.get_uint32();
                BITSTREAM_NOTE("version and usercode 0x"<< hex << setw(8) << setfill('0') << chip->usercode);
                break;
            case BitstreamCommand::CFG_1:
                if (!chip) {
                    // al3_s10 device bitstreams do not have deviceid set
                    chip = boost::make_optional(Chip(0x12006c31));
                    chip->metadata = metadata;
                }   
                BITSTREAM_DEBUG("CFG_1");
                chip->cfg1 = rd.get_uint32();
                break;
            case BitstreamCommand::CFG_2:
                BITSTREAM_DEBUG("CFG_2");
                chip->cfg2 = rd.get_uint32();
                break;
            case BitstreamCommand::FRAMES: {
                uint16_t frames = rd.get_uint16();
                uint32_t bits_per_frame = rd.get_uint16() << 3;
                BITSTREAM_NOTE("frames " << dec << frames << " bits_per_frame " << dec << bits_per_frame);
                BITSTREAM_NOTE("num_frames " << dec << chip->info.num_frames << " bits_per_frame " << dec << chip->info.bits_per_frame);
                if (frames != chip->info.num_frames)
                    throw BitstreamParseError("different number of frames than expected");
                if (bits_per_frame != chip->info.bits_per_frame)
                    throw BitstreamParseError("different bits per frame than expected");
                break;
            }
            case BitstreamCommand::MEM_FRAME: {
                rd.get_uint16();
                uint32_t bram_bits_per_frame = rd.get_uint16() << 3;
                if (bram_bits_per_frame != chip->info.bram_bits_per_frame)
                    throw BitstreamParseError("different BRAM bits per frame than expected");
                BITSTREAM_NOTE("bram_bits_per_frame " << dec << bram_bits_per_frame);
                break;
            }
            case BitstreamCommand::PROGRAM_DONE:
                BITSTREAM_NOTE("program done");
                rd.get_uint16();
                break;
            case BitstreamCommand::CMD_F3:
                BITSTREAM_DEBUG("CMD_F3");
                rd.skip_bytes(cmd_size-2);
                break;
            case BitstreamCommand::CMD_F5:
                BITSTREAM_DEBUG("CMD_F5");
                rd.get_uint16();
                break;
            case BitstreamCommand::CMD_C4:
                BITSTREAM_DEBUG("CMD_C4");
                if (!is_cpld)
                    rd.get_uint32();
                else {
                    rd.get_uint16();
                }
                break;
            case BitstreamCommand::CMD_C5:
                BITSTREAM_DEBUG("CMD_C5");
                rd.get_uint32();
                break;
            case BitstreamCommand::CMD_CA:
                BITSTREAM_DEBUG("CMD_CA");
                rd.get_uint32();
                break;
            case BitstreamCommand::FUSE_DATA: {
                uint16_t frames = rd.get_uint16();
                uint16_t bytes_per_frame = chip->info.bits_per_frame / 8L;
                unique_ptr<uint8_t[]> frame_bytes = make_unique<uint8_t[]>(bytes_per_frame);
                // taking current CRC16
                data_crc16.crc16 = rd.crc16.crc16;
                for (int idx = 0; idx < frames; idx++) {
                    block_size = rd.get_block_size();
                    rd.get_bytes(frame_bytes.get(), bytes_per_frame);
                    // Update CRC16 for complete frame
                    for (size_t i = 0; i < bytes_per_frame; i++) {
                        data_crc16.update_crc16(frame_bytes[i]);
                    }
                    uint16_t actual_crc = data_crc16.finalise_crc16();
                    uint16_t exp_crc = rd.get_uint16(); // crc 
                    if (actual_crc != exp_crc) {
                        ostringstream err;
                        err << "crc fail, calculated 0x" << hex << actual_crc << " but expecting 0x" << exp_crc;
                        throw BitstreamParseError(err.str());
                    }
                                      
                    if (rd.get_uint32()) 
                        throw BitstreamParseError("error parsing fuse data");
                    for (uint32_t j = 0; j < chip->info.bits_per_frame; j++) {
                        chip->cram.bit(idx, j) = (char) ((frame_bytes[(j / 8)]<< (j % 8)) & 0x80);
                    }
                    data_crc16.reset_crc16();    
                }
                // zero block
                block_size = rd.get_block_size();
                rd.skip_bytes(block_size);
                break;
            }
            case BitstreamCommand::MEMORY_DATA: {
                rd.get_uint16();
                uint8_t bram_block_id = rd.get_byte();
                // taking current CRC16
                data_crc16.crc16 = rd.crc16.crc16;
                BITSTREAM_NOTE("bram_block_id 0x" << hex << setw(2) << setfill('0') << (int)bram_block_id);
                uint16_t bram_bytes_per_frame = chip->info.bram_bits_per_frame / 8L;
                unique_ptr<uint8_t[]> frame_bytes = make_unique<uint8_t[]>(bram_bytes_per_frame);
                chip->bram_data[bram_block_id].resize(bram_bytes_per_frame);
                rd.get_bytes(frame_bytes.get(), bram_bytes_per_frame);
                // Update CRC16 for complete frame
                for (size_t i = 0; i < bram_bytes_per_frame; i++) {
                    data_crc16.update_crc16(frame_bytes[i]);
                }
                for(size_t i=0;i<bram_bytes_per_frame;i++)
                    chip->bram_data[bram_block_id][i] = frame_bytes[i];
                uint16_t actual_crc = data_crc16.finalise_crc16();
                uint16_t exp_crc = rd.get_uint16(); // crc 
                if (actual_crc != exp_crc) {
                    ostringstream err;
                    err << "crc fail, calculated 0x" << hex << actual_crc << " but expecting 0x" << exp_crc;
                    throw BitstreamParseError(err.str());
                }
                rd.skip_bytes(4); // padding
                break;
            }
            // CPLD specific
            case BitstreamCommand::RESET_CRC_CPLD:
                BITSTREAM_DEBUG("reset crc");
                rd.get_byte();
                data_crc16.reset_crc16();
                break;
            case BitstreamCommand::DEVICEID_CPLD: {
                uint32_t id = rd.get_uint32();
                BITSTREAM_NOTE("device ID: 0x" << hex << setw(8) << setfill('0') << id);
                chip = boost::make_optional(Chip(id));
                chip->metadata = metadata;
                is_cpld = true;
                break;
            }

            case BitstreamCommand::CMD_A1:
            case BitstreamCommand::CMD_A3:
            case BitstreamCommand::CMD_AC:
            case BitstreamCommand::CMD_B1:
                rd.skip_bytes(block_size-3-3);
                break;
            case BitstreamCommand::CPLD_DATA: {
                uint16_t frames = cmd_size;
                uint16_t bytes_per_frame = chip->info.bits_per_frame / 8L;
                unique_ptr<uint8_t[]> frame_bytes = make_unique<uint8_t[]>(bytes_per_frame);
                // taking current CRC16
                data_crc16.crc16 = rd.crc16.crc16;
                for (int idx = 0; idx < frames; idx++) {
                    block_size = rd.get_block_size();
                    rd.get_bytes(frame_bytes.get(), bytes_per_frame);
                    // Update CRC16 for complete frame
                    for (size_t i = 0; i < bytes_per_frame; i++) {
                        data_crc16.update_crc16(frame_bytes[i]);
                    }
                    uint16_t actual_crc = data_crc16.finalise_crc16();
                    uint16_t exp_crc = rd.get_uint16(); // crc 
                    if (actual_crc != exp_crc) {
                        ostringstream err;
                        err << "crc fail, calculated 0x" << hex << actual_crc << " but expecting 0x" << exp_crc;
                        printf("%s\n",err.str().c_str());
                        throw BitstreamParseError(err.str());
                    }
                                      
                    for (uint32_t j = 0; j < chip->info.bits_per_frame; j++) {
                        chip->cram.bit(idx, j) = (char) ((frame_bytes[(j / 8)]<< (j % 8)) & 0x80);
                    }
                    data_crc16.reset_crc16();    
                }
                break;
            }

            default:
                BITSTREAM_FATAL("unsupported command 0x" << hex << setw(2) << setfill('0') << int(cmd), rd.get_offset());
        }

        if (cmd!=BitstreamCommand::FUSE_DATA && cmd!=BitstreamCommand::CPLD_DATA && cmd!=BitstreamCommand::MEMORY_DATA) {
            rd.check_crc16();
        }
    }

    if (chip) {
        return *chip;
    } else {
        throw BitstreamParseError("failed to parse bitstream, no valid payload found");
    }
}

void Bitstream::write_fuse(const Chip &chip, std::ostream &file)
{
    for (int idx = 0; idx < chip.cram.frames(); idx++) {
        for (int pos = 0; pos < chip.cram.bits(); pos++) {
            file << chip.cram.get_bit(idx, pos);
        }
        file << std::endl;
    }
}

BitstreamParseError::BitstreamParseError(const std::string &desc) : runtime_error(desc.c_str()), desc(desc), offset(-1)
{
}

BitstreamParseError::BitstreamParseError(const std::string &desc, size_t offset)
        : runtime_error(desc.c_str()), desc(desc), offset(int(offset))
{
}

const char *BitstreamParseError::what() const noexcept
{
    std::ostringstream ss;
    ss << "Bitstream Parse Error: ";
    ss << desc;
    if (offset != -1)
        ss << " [at 0x" << std::hex << offset << "]";
    return strdup(ss.str().c_str());
}
} // namespace Tang
