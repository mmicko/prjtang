#include "Bitstream.hpp"
#include <cstring>

namespace Tang {

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

std::string Bitstream::vector_to_string(const std::vector<uint8_t> &data)
{
    std::ostringstream os;
    for (size_t i = 0; i < data.size(); i++) {
        os << std::hex << std::setw(2) << std::setfill('0') << int(data[i]);
    }
    return os.str();
}

void Bitstream::parse_command(const uint8_t command, const uint16_t size, const std::vector<uint8_t> &data,
                              const uint16_t crc16)
{
    switch (command) {
    case 0xf0: // JTAG ID
        printf("0xf0 DEVICEID:%s\n", vector_to_string(data).c_str());
        break;
    case 0xf1:
    case 0xf3:
    case 0xf7:
    case 0xc1:
    case 0xc2:
    case 0xc3:
    case 0xc5:
    case 0xc7:
    case 0xc8:
    case 0xca:
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
    switch (data[0]) {
    case 0xff: // all 0xff header
        break;
    case 0xcc:
        if (data[1] == 0x55 && data[2] == 0xaa && data[3] == 0x33) {
            // proper header
        }
        break;
    case 0xec:
        if (data[1] == 0xf0) {
            // printf("blocks %02x%02x\n",data[2],data[3]);
            data_blocks = (data[2] << 8) + data[3] + 1;
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
        if (flags != 0)
            throw BitstreamParseError("Byte after command should be zero");
        parse_command(data[0], size, std::vector<uint8_t>(data.begin() + 4, data.begin() + 4 + size - 2), crc16);

        /*printf("%04x\n",size);
        for(size_t i=4;i<4+size-2;i++) {
            printf("%02x",data[i]);
        }
        printf("\n");
        printf("%04x\n",crc16);*/
    } break;
    default:
        break;
    }
}

void Bitstream::parse()
{
    size_t pos = 0;
    data_blocks = 0;
    do {
        uint16_t len = (data[pos++] << 8);
        len += data[pos++];
        if ((len & 7) != 0)
            throw BitstreamParseError("Invalid size value in bitstream");
        len >>= 3;
        if ((pos + len) > data.size())
            throw BitstreamParseError("Invalid data in bitstream");

        if (data_blocks == 0) {
            parse_block(std::vector<uint8_t>(data.begin() + pos, data.begin() + pos + len));
        } else {
            data_blocks--;
        }

        pos += len;
    } while (pos < data.size());
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
}