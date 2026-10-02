#ifndef PACKET_DECODER_H
#define PACKET_DECODER_H

#include <cstdint>
#include <cstring>

#include "common.h"
#include "protocol.h"

namespace openflash
{
    namespace gba
    {
        bool decode_packet(const uint8_t *bytes, const size_t& size, protocol &protocol_);
    }
}

#endif // PACKET_DECODER_H