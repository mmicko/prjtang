#ifndef LIBTANG_BITSTREAM_HPP
#define LIBTANG_BITSTREAM_HPP

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Tang {

class Bitstream
{
  public:
    static Bitstream read(std::istream &in);

    void parse();
    void parse_block(const std::vector<uint8_t> &data);
    void parse_command(const uint8_t command, const uint16_t size, const std::vector<uint8_t> &data,
                       const uint16_t crc16);
    void parse_command_cpld(const uint8_t command, const uint16_t size, const std::vector<uint8_t> &data,
                            const uint16_t crc16);

  private:
    Bitstream(const std::vector<uint8_t> &data, const std::vector<std::string> &metadata);

    std::string vector_to_string(const std::vector<uint8_t> &data);

    uint16_t data_blocks;
    // Raw bitstream data
    std::vector<uint8_t> data;
    // BIT file metadata
    std::vector<std::string> metadata;
    // status if bitstream is from CPLD
    bool cpld;

    uint16_t rows;
    uint16_t row_bytes;
};

class BitstreamParseError : std::runtime_error
{
  public:
    explicit BitstreamParseError(const std::string &desc);

    BitstreamParseError(const std::string &desc, size_t offset);

    const char *what() const noexcept override;

  private:
    std::string desc;
    int offset;
};
}
#endif // LIBTANG_BITSTREAM_HPP
