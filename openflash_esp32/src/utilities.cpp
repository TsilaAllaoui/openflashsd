#include <string>
#include <vector>
#include <cstdint>

#include "utilities.h"

namespace openflash
{
    namespace esp32
    {
        void append_u8(std::vector<uint8_t> &payload, uint8_t value)
        {
            payload.emplace_back(value);
        }

        void append_u16(std::vector<uint8_t> &payload, uint16_t value)
        {
            payload.emplace_back(value & 0xFF);
            payload.emplace_back((value >> 8) & 0xFF);
        }

        void append_string(std::vector<uint8_t> &payload, const std::string &value)
        {
            append_u16(payload, static_cast<uint16_t>(value.size()));

            payload.insert(payload.end(), value.begin(), value.end());
        }

        bool read_u16(const std::vector<uint8_t> &payload, size_t &offset, uint16_t &value)
        {
            if (offset + 2 > payload.size())
                return false;

            value = static_cast<uint16_t>(payload[offset]) | (static_cast<uint16_t>(payload[offset + 1]) << 8);

            offset += 2;

            return true;
        }

        bool read_string(const std::vector<uint8_t> &payload, size_t &offset, std::string &value)
        {
            uint16_t length;

            if (!read_u16(payload, offset, length))
            {
                return false;
            }

            if (offset + length > payload.size())
                return false;

            value.assign(payload.begin() + offset, payload.begin() + offset + length);

            offset += length;

            return true;
        }

        void append_u32(std::vector<uint8_t> &payload, uint32_t value)
        {
            payload.emplace_back(value & 0xFF);

            payload.emplace_back((value >> 8) & 0xFF);

            payload.emplace_back((value >> 16) & 0xFF);

            payload.emplace_back((value >> 24) & 0xFF);
        }
    }
}