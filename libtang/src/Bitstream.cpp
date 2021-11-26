#include "Bitstream.hpp"
#include "Chip.hpp"
#include <bitset>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/optional.hpp>
#include <cstring>
#include <iostream>

namespace Tang {

// Add a single byte to the running CRC16 accumulator
void Crc16::update_crc16(uint8_t val)
{
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

uint16_t Crc16::finalise_crc16()
{
    // item b) "push out" the last 16 bits
    uint16_t crc = crc16;
    int i;
    bool bit_flag;
    for (i = 0; i < 16; ++i) {
        bit_flag = bool(crc >> 15);
        crc <<= 1;
        if (bit_flag)
            crc ^= CRC16_POLY;
    }

    return crc;
}

void Crc16::reset_crc16(uint16_t init) { crc16 = init; }

uint16_t Crc16::calc(const std::vector<uint8_t> &data, int start, int end)
{
    reset_crc16(CRC16_INIT);
    for (int i = start; i < end; i++)
        update_crc16(data[i]);
    return finalise_crc16();
}

uint16_t Crc16::update_block(const std::vector<uint8_t> &data, int start, int end)
{
    for (int i = start; i < end; i++)
        update_crc16(data[i]);
    return finalise_crc16();
}

Bitstream::Bitstream(const std::vector<uint8_t> &data, const std::vector<std::string> &metadata)
        : data(data), metadata(metadata), cpld(false), fuse_started(false), deviceid(0x12006c31)
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

std::string Bitstream::vector_to_string(const std::vector<uint8_t> &data)
{
    std::ostringstream os;
    for (size_t i = 0; i < data.size(); i++) {
        os << std::hex << std::setw(2) << std::setfill('0') << int(data[i]);
    }
    return os.str();
}

#define BIN(byte)                                                                                                      \
    (byte & 0x80 ? '1' : '0'), (byte & 0x40 ? '1' : '0'), (byte & 0x20 ? '1' : '0'), (byte & 0x10 ? '1' : '0'),        \
            (byte & 0x08 ? '1' : '0'), (byte & 0x04 ? '1' : '0'), (byte & 0x02 ? '1' : '0'), (byte & 0x01 ? '1' : '0')

void Bitstream::parse_command(const uint8_t command, const uint16_t size, const std::vector<uint8_t> &data,
                              const uint16_t crc16)
{
    switch (command) {
    case 0xf0: // JTAG ID
        if (verbose)
            printf("0xf0 DEVICEID:%s\n", vector_to_string(data).c_str());
        deviceid = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
        break;
    case 0xc1:
        if (verbose)
            printf("0xc1 VERSION:%02x UCODE:00000000%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", data[0],
                   BIN(data[1]), BIN(data[2]), BIN(data[3]));
        break;
    case 0xc2:
        if (verbose)
            printf("0xc2 hswapen %c mclk_freq_div %c%c%c%c%c unk %c%c active_done %c unk %c%c%c cascade_mode %c%c "
                   "security "
                   "%c unk %c auto_clear_en %c unk %c%c persist_bit %c UNK %c%c%c%c%c%c%c%c%c%c%c close_osc %c\n",
                   BIN(data[0]), BIN(data[1]), BIN(data[2]), BIN(data[3]));
        break;
    case 0xc3:
        if (verbose)
            printf("0xc3 UNK %c%c%c%c%c PLL %c%c%c%c unk %c%c%c done_sync %c pll_lock_wait %c%c%c%c done_phase %c%c%c "
                   "goe_phase %c%c%c gsr_phase %c%c%c gwd_phase %c%c%c usr_gsrn_en %c gsrn_sync_sel %c  UNK %c\n",
                   BIN(data[0]), BIN(data[1]), BIN(data[2]), BIN(data[3]));
        break;
    case 0xc7:
        if (verbose)
            printf("0xc7 FRAMES:%d BYTES_PER_FRAME:%d (%d bits)\n", (data[0] * 256 + data[1]),
                   (data[2] * 256 + data[3]), (data[2] * 256 + data[3]) * 8);
        frames = data[0] * 256 + data[1];
        frame_bytes = data[2] * 256 + data[3];
        break;
    case 0xc8:
        if (verbose)
            printf("0xc8 BYTES_PER_MEM_FRAME:%d (%d bits)\n", (data[2] * 256 + data[3]), (data[2] * 256 + data[3]) * 8);
        mem_frame_bytes = data[2] * 256 + data[3];
        break;

    case 0xf1:
        if (verbose)
            printf("0xf1 set CRC16 to :%04x\n", (data[0] * 256 + data[1]));
        crc.reset_crc16(data[0] * 256 + data[1]);
        break;
    case 0xf7:
        if (verbose)
            printf("0xf7 DONE\n");
        break;
    case 0xf3:
    case 0xc4:
    case 0xc5:
    case 0xca:
        if (verbose)
            printf("0x%02x [%04x] [crc %04x]:%s \n", command, size, crc16, vector_to_string(data).c_str());
        break;
    default:
        std::ostringstream os;
        os << "Unknown command in bitstream " << std::hex << std::setw(2) << std::setfill('0') << int(command);
        throw BitstreamParseError(os.str());
    }
}

void Bitstream::parse_command_cpld(const uint8_t command, const uint16_t size, const std::vector<uint8_t> &data,
                                   const uint16_t crc16)
{
    switch (command) {
    case 0x90: // JTAG ID
        cpld = true;
        if (verbose)
            printf("0x90 DEVICEID:%s\n", vector_to_string(std::vector<uint8_t>(data.begin() + 3, data.end())).c_str());
        deviceid = (data[3] << 24) + (data[4] << 16) + (data[5] << 8) + data[6];
        frame_bytes = 86; // for elf_3/6
        break;
    case 0xa8:
        if (verbose)
            printf("0xa8 set CRC16 to :%04x\n", (data[2] * 256 + data[3]));
        crc.reset_crc16(data[2] * 256 + data[3]);
        break;
    case 0xa1:
    case 0xa3:
    case 0xac:
    case 0xb1:
    case 0xc4:
        if (verbose)
            printf("0x%02x [%04x] [crc %04x]:%s \n", command, size, crc16, vector_to_string(data).c_str());
        break;
    default:
        std::ostringstream os;
        os << "Unknown command in bitstream " << std::hex << std::setw(2) << std::setfill('0') << int(command);
        throw BitstreamParseError(os.str());
    }
}

void Bitstream::parse_block(const std::vector<uint8_t> &data)
{
    Crc16 crc;
    if (verbose)
        printf("block:%s\n", vector_to_string(data).c_str());
    switch (data[0]) {
    // Common section
    case 0xff: // all 0xff header
        break;
    case 0xcc:
        if (data[1] == 0x55 && data[2] == 0xaa && data[3] == 0x33) {
            // proper header
        }
        break;

    case 0xc4: {
        if (cpld) {
            uint16_t size = data.size() - 3;
            uint16_t crc16 = (data[data.size() - 2] << 8) + data[data.size() - 1];
            uint16_t calc_crc16 = crc.calc(data, 0, data.size() - 2);
            if (crc16 != calc_crc16)
                throw BitstreamParseError("CRC16 error");
            parse_command_cpld(data[0], size, std::vector<uint8_t>(data.begin() + 1, data.begin() + 1 + size), crc16);
        } else {
            uint8_t flags = data[1];
            uint16_t size = (data[2] << 8) + data[3];
            uint16_t crc16 = (data[4 + size - 2] << 8) + data[4 + size - 1];
            uint16_t calc_crc16 = crc.calc(data, 0, data.size() - 2);
            if (crc16 != calc_crc16)
                throw BitstreamParseError("CRC16 error");
            if (flags != 0)
                throw BitstreamParseError("Byte after command should be zero");
            parse_command(data[0], size, std::vector<uint8_t>(data.begin() + 4, data.begin() + 4 + size - 2), crc16);
        }
    } break;
    // CPLD section
    case 0xaa:
        if (data[1] == 0x00) {
            data_blocks = (data[2] << 8) + data[3];
            if (verbose)
                printf("0xaa FRAMES:%d\n", (data[2] * 256 + data[3]));
        }
        break;
    case 0xac:
    case 0xb1:
    case 0x90:
    case 0xa1:
    case 0xa8:
    case 0xa3: {
        uint16_t size = data.size() - 3;
        uint16_t crc16 = (data[data.size() - 2] << 8) + data[data.size() - 1];
        uint16_t calc_crc16 = crc.calc(data, 0, data.size() - 2);
        if (crc16 != calc_crc16)
            throw BitstreamParseError("CRC16 error");
        parse_command_cpld(data[0], size, std::vector<uint8_t>(data.begin() + 1, data.begin() + 1 + size), crc16);
    } break;

    // FPGA section
    case 0xec:
        if (data[1] == 0xf0) {
            data_blocks = (data[2] << 8) + data[3] + 1;
            if (((data[2] << 8) + data[3]) == frames)
                fuse_started = true;
        }
        break;
    case 0xf0:
    case 0xf1:
    case 0xf3:
    case 0xf7:
    case 0xc1:
    case 0xc2:
    case 0xc3:
    case 0xc5:
    case 0xc7:
    case 0xc8:
    case 0xca: {
        uint8_t flags = data[1];
        uint16_t size = (data[2] << 8) + data[3];
        uint16_t crc16 = (data[4 + size - 2] << 8) + data[4 + size - 1];
        uint16_t calc_crc16 = crc.calc(data, 0, data.size() - 2);
        if (crc16 != calc_crc16)
            throw BitstreamParseError("CRC16 error");
        if (flags != 0)
            throw BitstreamParseError("Byte after command should be zero");
        parse_command(data[0], size, std::vector<uint8_t>(data.begin() + 4, data.begin() + 4 + size - 2), crc16);
    } break;
    default:
        break;
    }
}

void Bitstream::parse(bool verbose_info, bool verbose_data)
{
    size_t pos = 0;
    data_blocks = 0;
    verbose = verbose_info;
    crc.reset_crc16();
    do {
        uint16_t len = (data[pos++] << 8);
        len += data[pos++];
        if ((len & 7) != 0)
            throw BitstreamParseError("Invalid size value in bitstream");
        len >>= 3;
        if ((pos + len) > data.size())
            throw BitstreamParseError("Invalid data in bitstream");

        std::vector<uint8_t> block = std::vector<uint8_t>(data.begin() + pos, data.begin() + pos + len);
        blocks.push_back(block);
        if (data_blocks == 0) {
            crc.update_block(block, 0, block.size());
            parse_block(block);
            if (fuse_started) {
                fuse_start_block = blocks.size();
                fuse_started = false;
            }
        } else {
            if (verbose_data)
                printf("data:%s\n", vector_to_string(block).c_str());
            if (frame_bytes > block.size()) {
                crc.update_block(block, 0, block.size());
            } else {
                uint16_t crc_calc = crc.update_block(block, 0, frame_bytes);
                uint16_t crc_file = block[frame_bytes] * 256 + block[frame_bytes + 1];
                if (crc_calc != crc_file)
                    throw BitstreamParseError("CRC16 error");
                crc.update_block(block, frame_bytes, block.size());
            }
            data_blocks--;
        }

        pos += len;
    } while (pos < data.size());
}

uint16_t Bitstream::calculate_bitstream_crc()
{
    crc.reset_crc16();
    for (const auto &block : blocks) {
        crc.update_block(block, 0, block.size());
    }
    return crc.finalise_crc16();
}

void Bitstream::write_bin(std::ostream &file)
{
    for (const auto &block : blocks) {
        file.write((const char *)&block[0], block.size());
    }
}

void Bitstream::write_bas(std::ostream &file)
{
    for (const auto &meta : metadata) {
        file << meta << std::endl;
    }
    for (const auto &block : blocks) {
        for (size_t pos = 0; pos < block.size(); pos++) {
            file << std::bitset<8>(block[pos]);
        }
        file << std::endl;
    }
}

void Bitstream::write_bmk(std::ostream &file)
{
    for (const auto &meta : metadata) {
        if (boost::starts_with(meta, "# Bitstream CRC:"))
            continue;
        if (boost::starts_with(meta, "# USER CODE:"))
            continue;
        file << meta << std::endl;
    }
    for (auto it = blocks.begin(); it != (blocks.begin() + fuse_start_block); ++it) {
        uint16_t size = it->size() << 3;
        file << uint8_t(size >> 8) << uint8_t(size & 0xff);
        for (size_t pos = 0; pos < it->size(); pos++) {
            file << ((*it)[pos]);
        }
    }
    for (auto it = blocks.begin() + fuse_start_block; it != (blocks.begin() + fuse_start_block + frames + 1); ++it) {
        uint16_t size = it->size() << 3;
        file << uint8_t(size >> 8) << uint8_t(size & 0xff);
        for (size_t pos = 0; pos < it->size(); pos++) {
            file << uint8_t(0);
        }
    }
    for (auto it = blocks.begin() + fuse_start_block + frames + 1; it != blocks.end(); ++it) {
        if ((*it)[0] == 0)
            break;
        uint16_t size = it->size() << 3;
        file << uint8_t(size >> 8) << uint8_t(size & 0xff);
        for (size_t pos = 0; pos < it->size(); pos++) {
            file << (*it)[pos];
        }
    }
}

void Bitstream::write_bma(std::ostream &file)
{
    for (const auto &meta : metadata) {
        if (boost::starts_with(meta, "# Bitstream CRC:"))
            continue;
        if (boost::starts_with(meta, "# USER CODE:"))
            continue;
        file << meta << std::endl;
    }
    for (auto it = blocks.begin(); it != (blocks.begin() + fuse_start_block); ++it) {
        for (size_t pos = 0; pos < it->size(); pos++) {
            file << std::bitset<8>((*it)[pos]);
        }
        file << std::endl;
    }
    for (auto it = blocks.begin() + fuse_start_block; it != (blocks.begin() + fuse_start_block + frames + 1); ++it) {
        for (size_t pos = 0; pos < it->size(); pos++) {
            file << std::bitset<8>(0);
        }
        file << std::endl;
    }
    for (auto it = blocks.begin() + fuse_start_block + frames + 1; it != blocks.end(); ++it) {
        if ((*it)[0] == 0)
            break;
        for (size_t pos = 0; pos < it->size(); pos++) {
            file << std::bitset<8>((*it)[pos]);
        }
        file << std::endl;
    }
}

void Bitstream::write_fuse(std::ostream &file)
{
    for (auto it = (blocks.begin() + fuse_start_block); it != (blocks.begin() + fuse_start_block + frames); ++it) {
        for (size_t pos = 0; pos < frame_bytes; pos++) {
            file << std::bitset<8>((*it)[pos]);
        }
        file << std::endl;
    }
}

uint8_t reverse_byte(uint8_t byte)
{
    uint8_t rev = 0;
    for (int i = 0; i < 8; i++)
        if (byte & (1 << i))
            rev |= (1 << (7 - i));
    return rev;
}

void Bitstream::write_svf(std::ostream &file)
{
    file << "// Created using Project Tang Software" << std::endl;
    file << "// Date: 2018/12/27 14:58" << std::endl;
    file << "// Architecture: eagle_s20" << std::endl;
    file << "// Package: BG256" << std::endl;
    file << std::endl;
    file << "TRST OFF;" << std::endl;
    file << "ENDIR IDLE;" << std::endl;
    file << "ENDDR IDLE;" << std::endl;
    file << "STATE RESET;" << std::endl;
    file << "STATE IDLE;" << std::endl;
    file << "FREQUENCY 1E6 HZ;" << std::endl;
    // Operation: Program
    file << "TIR 0 ;" << std::endl;
    file << "HIR 0 ;" << std::endl;
    file << "TDR 0 ;" << std::endl;
    file << "HDR 0 ;" << std::endl;
    file << "TIR 0 ;" << std::endl;
    file << "HIR 0 ;" << std::endl;
    file << "HDR 0 ;" << std::endl;
    file << "TDR 0 ;" << std::endl;
    // Loading device with 'idcode' instruction.
    file << "SIR 8 TDI (06) SMASK (ff) ;" << std::endl;
    file << "RUNTEST 15 TCK;" << std::endl;
    file << "SDR 32 TDI (00000000) SMASK (ffffffff) TDO (" << std::setw(8) << std::hex << std::setfill('0') << deviceid
         << ") MASK (ffffffff) ;" << std::endl;
    // Boundary Scan Chain Contents
    // Position 1: BG256
    file << "TIR 0 ;" << std::endl;
    file << "HIR 0 ;" << std::endl;
    file << "TDR 0 ;" << std::endl;
    file << "HDR 0 ;" << std::endl;
    file << "TIR 0 ;" << std::endl;
    file << "HIR 0 ;" << std::endl;
    file << "TDR 0 ;" << std::endl;
    file << "HDR 0 ;" << std::endl;
    file << "TIR 0 ;" << std::endl;
    file << "HIR 0 ;" << std::endl;
    file << "HDR 0 ;" << std::endl;
    file << "TDR 0 ;" << std::endl;
    // Loading device with 'idcode' instruction.
    file << "SIR 8 TDI (06) SMASK (ff) ;" << std::endl;
    file << "SDR 32 TDI (00000000) SMASK (ffffffff) TDO (" << std::setw(8) << std::hex << std::setfill('0') << deviceid
         << ") MASK (ffffffff) ;" << std::endl;
    // Loading device with 'refresh' instruction.
    file << "SIR 8 TDI (01) SMASK (ff) ;" << std::endl;
    // Loading device with 'bypass' instruction.
    file << "SIR 8 TDI (1f) ;" << std::endl;
    file << "SIR 8 TDI (39) ;" << std::endl;
    file << "RUNTEST 50000 TCK;" << std::endl;
    // Loading device with 'jtag program' instruction.
    file << "SIR 8 TDI (30) SMASK (ff) ;" << std::endl;
    file << "RUNTEST 15 TCK;" << std::endl;
    // Loading device with a `cfg_in` instruction.
    file << "SIR 8 TDI (3b) SMASK (ff) ;" << std::endl;
    file << "RUNTEST 15 TCK;" << std::endl;

    size_t count = 0;
    for (auto const &block : blocks) {
        count += block.size();
    }
    // Begin of bitstream data
    file << "SDR " << std::dec << int(count * 8) << " TDI (" << std::endl;
    int i = 0;
    for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
        for (size_t pos = 0; pos < it->size(); pos++) {
            file << std::hex << std::setw(2) << std::setfill('0') << int(reverse_byte(((*it)[it->size() - pos - 1])));
            i++;
            if ((i % 1024) == 0)
                file << std::endl;
        }
    }
    file << std::endl << ") SMASK (" << std::endl;
    for (size_t j = 1; j < count + 1; j++) {
        file << "ff";
        if ((j % 1024) == 0)
            file << std::endl;
    }
    file << std::endl << ") ;" << std::endl;

    file << "RUNTEST 100 TCK;" << std::endl;
    // Loading device with a `jtag start` instruction.
    file << "SIR 8 TDI (3d) ;" << std::endl;
    file << "RUNTEST 15 TCK;" << std::endl;
    // Loading device with 'bypass' instruction.
    file << "SIR 8 TDI (1f) ;" << std::endl;
    file << "RUNTEST 1000 TCK;" << std::endl;
    file << "TIR 0 ;" << std::endl;
    file << "HIR 0 ;" << std::endl;
    file << "HDR 0 ;" << std::endl;
    file << "TDR 0 ;" << std::endl;
    file << "TIR 0 ;" << std::endl;
    file << "HIR 0 ;" << std::endl;
    file << "HDR 0 ;" << std::endl;
    file << "TDR 0 ;" << std::endl;
    // Loading device with 'bypass' instruction.
    file << "SIR 8 TDI (1f) ;" << std::endl;
}

Chip Bitstream::deserialise_chip()
{
    boost::optional<Chip> chip;
/*    cerr << "bitstream size: " << data.size() * 8 << " bits" << endl;
    BitstreamReadWriter rd(data);
    boost::optional<Chip> chip;
    bool found_preamble = rd.find_preamble(preamble);
    boost::optional<array<uint8_t, 8>> compression_dict;

    if (!found_preamble)
        throw BitstreamParseError("preamble not found in bitstream");

    uint16_t current_ebr = 0;
    int addr_in_ebr = 0;

    while (!rd.is_end()) {
        BitstreamCommand cmd = rd.get_command_opcode();
        switch (cmd) {
            case BitstreamCommand::LSC_RESET_CRC:
                BITSTREAM_DEBUG("reset crc");
                rd.skip_bytes(3);
                rd.reset_crc16();
                break;
            case BitstreamCommand::VERIFY_ID: {
                rd.skip_bytes(3);
                uint32_t id = rd.get_uint32();
                if (idcode) {
                    BITSTREAM_NOTE("Overriding device ID from 0x" << hex << setw(8) << setfill('0') << id << " to 0x" << *idcode);
                    id = *idcode;
                }

                BITSTREAM_NOTE("device ID: 0x" << hex << setw(8) << setfill('0') << id);
                chip = boost::make_optional(Chip(id));
                chip->metadata = metadata;
            }
                break;
            case BitstreamCommand::LSC_PROG_CNTRL0: {
                rd.skip_bytes(3);
                uint32_t cfg = rd.get_uint32();
                chip->ctrl0 = cfg;
                BITSTREAM_DEBUG("set control reg 0 to 0x" << hex << setw(8) << setfill('0') << cfg);
            }
                break;
            case BitstreamCommand::ISC_PROGRAM_DONE:
                rd.skip_bytes(3);
                BITSTREAM_NOTE("program DONE");
                break;
            case BitstreamCommand::ISC_PROGRAM_SECURITY:
                rd.skip_bytes(3);
                BITSTREAM_NOTE("program SECURITY");
                break;
            case BitstreamCommand::ISC_PROGRAM_USERCODE: {
                bool check_crc = (rd.get_byte() & 0x80) != 0;
                rd.skip_bytes(2);
                uint32_t uc = rd.get_uint32();
                BITSTREAM_NOTE("set USERCODE to 0x" << hex << setw(8) << setfill('0') << uc);
                chip->usercode = uc;
                if (check_crc)
                    rd.check_crc16();
            }
                break;
            case BitstreamCommand::LSC_WRITE_COMP_DIC: {
                bool check_crc = (rd.get_byte() & 0x80) != 0;
                rd.skip_bytes(2);
                compression_dict = boost::make_optional(array<uint8_t, 8>());
                // patterns are stored in the bitstream in reverse order: pattern7 to pattern0
                for (int i = 7; i >= 0; i--) {
                  uint8_t pattern = rd.get_byte();
                  compression_dict.get()[i] = pattern;
                }
                BITSTREAM_DEBUG("write compression dictionary: " <<
                                "0x" << hex << setw(2) << setfill('0') << int(compression_dict.get()[0]) << " " <<
                                "0x" << hex << setw(2) << setfill('0') << int(compression_dict.get()[1]) << " " <<
                                "0x" << hex << setw(2) << setfill('0') << int(compression_dict.get()[2]) << " " <<
                                "0x" << hex << setw(2) << setfill('0') << int(compression_dict.get()[3]) << " " <<
                                "0x" << hex << setw(2) << setfill('0') << int(compression_dict.get()[4]) << " " <<
                                "0x" << hex << setw(2) << setfill('0') << int(compression_dict.get()[5]) << " " <<
                                "0x" << hex << setw(2) << setfill('0') << int(compression_dict.get()[6]) << " " <<
                                "0x" << hex << setw(2) << setfill('0') << int(compression_dict.get()[7]));;
                if (check_crc)
                  rd.check_crc16();
            }
                break;
            case BitstreamCommand::LSC_INIT_ADDRESS:
                rd.skip_bytes(3);
                BITSTREAM_DEBUG("init address");
                break;
            case BitstreamCommand::LSC_PROG_INCR_CMP:
                // This is the main bitstream payload (compressed)
                BITSTREAM_DEBUG("Compressed bitstream found");
                if (!compression_dict)
                    throw BitstreamParseError("start of compressed bitstream data before compression dictionary was stored", rd.get_offset());
                // fall through
            case BitstreamCommand::LSC_PROG_INCR_RTI: {
                // This is the main bitstream payload
                if (!chip)
                    throw BitstreamParseError("start of bitstream data before chip was identified", rd.get_offset());

                BitstreamOptions ops(chip.get()); // Only reversed_frames is meaningful here.

                uint8_t params[3];
                rd.get_bytes(params, 3);
                BITSTREAM_DEBUG("settings: " << hex << setw(2) << int(params[0]) << " " << int(params[1]) << " "
                                             << int(params[2]));
                // I've only seen 0x81 for the ecp5 and 0x8e for the xo2 so far...
                bool check_crc = params[0] & 0x80U;
                // inverted value: a 0 means check after every frame
                bool crc_after_each_frame = check_crc && !(params[0] & 0x40U);
                // I don't know what these two are for I've seen both 1s (XO2) and both 0s (ECP5)
                // The names are from the ECP5 docs
                // bool include_dummy_bits = params[0] & 0x20U;
                // bool include_dummy_bytes = params[0] & 0x10U;
                size_t dummy_bytes = params[0] & 0x0FU;
                size_t frame_count = (params[1] << 8U) | params[2];
                BITSTREAM_NOTE("reading " << std::dec << frame_count << " config frames (with " << std::dec << dummy_bytes << " dummy bytes)");
                size_t bytes_per_frame = (chip->info.bits_per_frame + chip->info.pad_bits_after_frame +
                                          chip->info.pad_bits_before_frame) / 8U;
                // If compressed 0 bits are added to the stream before compression to make it 64 bit bounded, so
                // we should consider that space here but they shouldn't be copied to the output
                if (cmd == BitstreamCommand::LSC_PROG_INCR_CMP)
                    bytes_per_frame += (7 - ((bytes_per_frame - 1) % 8));
                unique_ptr<uint8_t[]> frame_bytes = make_unique<uint8_t[]>(bytes_per_frame);
                for (size_t i = 0; i < frame_count; i++) {
                    size_t idx = ops.reversed_frames ? (chip->info.num_frames - 1) - i : i;
                    if (cmd == BitstreamCommand::LSC_PROG_INCR_CMP)
                        rd.get_compressed_bytes(frame_bytes.get(), bytes_per_frame, compression_dict.get());
                    else
                        rd.get_bytes(frame_bytes.get(), bytes_per_frame);

                    for (int j = 0; j < chip->info.bits_per_frame; j++) {
                        size_t ofs = j + chip->info.pad_bits_after_frame;
                        chip->cram.bit(idx, j) = (char)
                            ((frame_bytes[(bytes_per_frame - 1) - (ofs / 8)] >> (ofs % 8)) & 0x01);
                    }
                    if (crc_after_each_frame || (check_crc && (i == frame_count-1)))
                      rd.check_crc16();
                    rd.skip_bytes(dummy_bytes);
                }
            }
                break;
            case BitstreamCommand::LSC_EBR_ADDRESS: {
                rd.skip_bytes(3);
                uint32_t data = rd.get_uint32();
                current_ebr = (data >> 11) & 0x3FF;
                addr_in_ebr = data & 0x7FF;
                chip->bram_data[current_ebr].resize(2048);
            }
                break;
            case BitstreamCommand::LSC_EBR_WRITE: {
                uint8_t params[3];
                rd.get_bytes(params, 3);
                int frame_count = (params[1] << 8U) | params[2];
                int frames_read = 0;

                while (frames_read < frame_count) {

                    if (addr_in_ebr >= 2048) {
                        addr_in_ebr = 0;
                        current_ebr++;
                        chip->bram_data[current_ebr].resize(2048);
                    }

                    auto &ebr = chip->bram_data[current_ebr];
                    frames_read++;
                    uint8_t frame[9];
                    rd.get_bytes(frame, 9);
                    ebr.at(addr_in_ebr+0) = (frame[0] << 1)        | (frame[1] >> 7);
                    ebr.at(addr_in_ebr+1) = (frame[1] & 0x7F) << 2 | (frame[2] >> 6);
                    ebr.at(addr_in_ebr+2) = (frame[2] & 0x3F) << 3 | (frame[3] >> 5);
                    ebr.at(addr_in_ebr+3) = (frame[3] & 0x1F) << 4 | (frame[4] >> 4);
                    ebr.at(addr_in_ebr+4) = (frame[4] & 0x0F) << 5 | (frame[5] >> 3);
                    ebr.at(addr_in_ebr+5) = (frame[5] & 0x07) << 6 | (frame[6] >> 2);
                    ebr.at(addr_in_ebr+6) = (frame[6] & 0x03) << 7 | (frame[7] >> 1);
                    ebr.at(addr_in_ebr+7) = (frame[7] & 0x01) << 8 | frame[8];
                    addr_in_ebr += 8;

                }
                rd.check_crc16();
            }
                break;
            case BitstreamCommand::SPI_MODE: {
                uint8_t spi_mode;
                rd.get_bytes(&spi_mode, 1);
                rd.skip_bytes(2);

                auto spimode = find_if(spi_modes.begin(), spi_modes.end(), [&](const pair<string, uint8_t> &fp){
                    return fp.second == spi_mode;
                });
                if (spimode == spi_modes.end())
                    throw runtime_error("bad SPI mode" + std::to_string(spi_mode));

                BITSTREAM_NOTE("SPI Mode " <<  spimode->first);
            }
                break;
            case BitstreamCommand::JUMP:
                rd.skip_bytes(3);
                BITSTREAM_DEBUG("Jump command");
                // TODO: Parse address and SPI Flash read speed 
                rd.skip_bytes(4);
                break;
            case BitstreamCommand::DUMMY:
                break;
            default: BITSTREAM_FATAL("unsupported command 0x" << hex << setw(2) << setfill('0') << int(cmd),
                                     rd.get_offset());
        }
    }
    */
    if (chip) {
        return *chip;
    } else {
        throw BitstreamParseError("failed to parse bitstream, no valid payload found");
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
