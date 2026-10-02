#ifndef CRC16_H
#define CRC16_H

#include <cstdint>
#include <cstring>

namespace openflash
{
    uint16_t calculate_crc16(const uint8_t *data, size_t size);
}

#endif // CRC16_H
