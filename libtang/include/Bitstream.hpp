#ifndef LIBTANG_BITSTREAM_HPP
#define LIBTANG_BITSTREAM_HPP

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Tang {

class Chip;

class Bitstream
{
  public:
    static Bitstream read(std::istream &in);

    void parse(bool verbose_info, bool verbose_data);
    void parse_block(const std::vector<uint8_t> &data);
    void parse_command(const uint8_t command, const uint16_t size, const std::vector<uint8_t> &data,
                       const uint16_t crc16);
    void parse_command_cpld(const uint8_t command, const uint16_t size, const std::vector<uint8_t> &data,
                            const uint16_t crc16);
    //uint16_t calculate_bitstream_crc();
    static void write_fuse(const Chip &chip, std::ostream &file);
    /*void write_bin(std::ostream &file);
    void write_bas(std::ostream &file);
    void write_bmk(std::ostream &file);
    void write_bma(std::ostream &file);
    void write_svf(std::ostream &file);
*/
    // Deserialise a bitstream to a Chip
    Chip deserialise_chip();
  private:
    Bitstream(const std::vector<uint8_t> &data, const std::vector<std::string> &metadata);

    std::string vector_to_string(const std::vector<uint8_t> &data);
    // Raw bitstream data
    std::vector<uint8_t> data;
    // BIT file metadata
    std::vector<std::string> metadata;

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
} // namespace Tang
#endif // LIBTANG_BITSTREAM_HPP
