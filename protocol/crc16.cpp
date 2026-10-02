#include "crc16.h"

namespace openflash
{
    uint16_t calculate_crc16(const uint8_t *data, size_t size)
    {
        uint16_t crc = 0xFFFF;

        for (size_t index = 0; index < size; ++index)
        {
            crc ^= static_cast<uint16_t>(data[index]) << 8;

            for (int bit = 0; bit < 8; ++bit)
            {
                if (crc & 0x8000)
                    crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
                else
                    crc <<= 1;
            }
        }

        return crc;
    }
}