#ifndef GBA_COMMUNICATION_UTILITIES_H
#define GBA_COMMUNICATION_UTILITIES_H

#include <cstdint>

#include "bn_optional.h"

#include "common.h"
#include "packet_stream_parser.h"

namespace openflash
{
    namespace gba
    {
        extern uint8_t sequence_number;
        extern uint8_t received_byte;
        extern openflash::gba::packet_stream_parser parser;

        void send_packet(const uint8_t *payload, uint16_t payload_size, uint8_t *output, openflash::command cmd);
        bool receive_packet(protocol &packet);
    }
}

#endif // GBA_COMMUNICATION_UTILITIES_H
