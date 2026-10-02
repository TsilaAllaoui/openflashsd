#ifndef PACKET_ENCODER_H
#define PACKET_ENCODER_H

#include <cstdint>
#include <cstring>

#include "common.h"

namespace openflash
{
    bool encode_packet(uint8_t sequence,
                       command command_,
                       const uint8_t *payload,
                       uint16_t payload_size,
                       uint8_t *output,
                       size_t output_capacity,
                       size_t &output_size);
}

#endif // PACKET_ENCODER_H