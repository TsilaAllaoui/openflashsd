#ifndef GBA_PROTOCOL_H
#define GBA_PROTOCOL_H

#include <cstdint>

#include "common.h"
#include "bn_vector.h"

namespace openflash
{
    namespace gba
    {
        struct protocol
        {
            uint8_t protocol_version = current_protocol_version;
            uint8_t sequence_number = 0;
            command cmd = command::PING;
            bn::vector<uint8_t, max_payload_size> payload;
        };
    }
}

#endif // GBA_PROTOCOL_H