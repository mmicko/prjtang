#ifndef LIBTANG_BITSTREAM_HPP
#define LIBTANG_BITSTREAM_HPP

#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <vector>
#include <stdexcept>

namespace Tang {
	
class Bitstream {
public:
    static Bitstream read(std::istream &in);
	
	void parse();
    void parse_block(const std::vector<uint8_t> &data);
    void parse_command(const uint8_t command, const uint16_t size, const std::vector<uint8_t> &data, const uint16_t crc16);
private:
	Bitstream(const std::vector<uint8_t> &data, const std::vector<std::string> &metadata);
	
    std::string vector_to_string(const std::vector<uint8_t> &data);

    uint16_t data_blocks;
    // Raw bitstream data
    std::vector<uint8_t> data;
    // Lattice BIT file metadata
    std::vector<std::string> metadata;
};

class BitstreamParseError : std::runtime_error {
public:
    explicit BitstreamParseError(const std::string &desc);

    BitstreamParseError(const std::string &desc, size_t offset);

    const char *what() const noexcept override;

private:
    std::string desc;
    int offset;
};

}
#endif //LIBTANG_BITSTREAM_HPP
