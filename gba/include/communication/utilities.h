#ifndef GBA_COMMUNICATION_UTILITIES_H
#define GBA_COMMUNICATION_UTILITIES_H

#include <cstdint>

#include "bn_optional.h"

#include "common.h"
#include "constants.h"
#include "packet_stream_parser.h"

namespace openflash
{
    namespace gba
    {
        extern uint8_t sequence_number;
        extern uint8_t received_byte;
        extern openflash::gba::packet_stream_parser parser;

        void send_packet(const uint8_t *payload, uint16_t payload_size, openflash::command cmd);
        bool receive_packet(protocol &packet);

        void append_u8(uint8_t *payload, uint8_t value);
        void append_u16(uint8_t *payload, uint16_t value);
        void append_string(uint8_t *payload, const bn::string<max_file_name_character_count> &value);
        bool read_u16(const uint8_t *payload, size_t &offset, uint16_t &value);
        bool read_string(const uint8_t *payload, size_t &offset, bn::string<max_file_name_character_count> &value);
        void append_u32(uint8_t *payload, uint32_t value);
        bool read_u32(const uint8_t *payload, size_t &offset, uint32_t &value);
    }
}

#endif // GBA_COMMUNICATION_UTILITIES_H
