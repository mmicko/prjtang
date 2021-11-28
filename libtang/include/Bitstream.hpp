#ifndef LIBTANG_BITSTREAM_HPP
#define LIBTANG_BITSTREAM_HPP

#include <cstdint>
#include <memory>
#include <iostream>


#include <vector>
#include <string>
#include <stdexcept>
#include <map>
#include <boost/optional.hpp>

namespace Tang {

class Chip;

class Bitstream
{
  public:
    static Bitstream read(std::istream &in);

    // Serialize Chip to bitstream 
    static void write_fuse(const Chip &chip, std::ostream &out);
    
    void write_bit(std::ostream &out);
    void write_bin(std::ostream &out);
    void write_bas(std::ostream &out);
    void write_bmk(const Chip &chip, std::ostream &out);
    void write_bma(const Chip &chip, std::ostream &out);
    void write_rbf(std::ostream &out);
    void write_svf(const Chip &chip, std::ostream &out);

    // Serialise a Chip back to a bitstream
    static Bitstream serialise_chip(const Chip &chip, const std::map<std::string, std::string> options);

    // Deserialise a bitstream to a Chip
    Chip deserialise_chip();
  private:
    Bitstream(const std::vector<uint8_t> &data, const std::vector<std::string> &metadata);

    std::string vector_to_string(const std::vector<uint8_t> &data);
    // Bitstream data parsed into blocks
    std::vector<uint8_t> data;
    std::vector<std::vector<uint8_t>> blocks;
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
