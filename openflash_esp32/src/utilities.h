#ifndef UTILITIES_H
#define UTILITIES_H

namespace openflash
{
    namespace esp32
    {
        void append_u8(std::vector<uint8_t> &payload, uint8_t value);
        void append_u16(std::vector<uint8_t> &payload, uint16_t value);
        void append_string(std::vector<uint8_t> &payload, const std::string &value);
        bool read_u16(const std::vector<uint8_t> &payload, size_t &offset, uint16_t &value);
        bool read_string(const std::vector<uint8_t> &payload, size_t &offset, std::string &value);
        void append_u32(std::vector<uint8_t> &payload, uint32_t value);
    }
}

#endif // UTILITIES_H