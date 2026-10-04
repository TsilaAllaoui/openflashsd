#include <cstdint>

#include "bn_string.h"

#include "common.h"
#include "protocol.h"
#include "utilities.h"
#include "gba_uart.h"
#include "packet_encoder.h"

namespace openflash
{
    namespace gba
    {
        uint8_t sequence_number = 0;
        uint8_t received_byte = 0;
        openflash::gba::packet_stream_parser parser;

        void send_packet(const uint8_t *payload, uint16_t payload_size, openflash::command cmd)
        {
            uint8_t output[max_packet_size];
            size_t encoded_size = 0;
            bool result = openflash::encode_packet(sequence_number,
                                                   cmd,
                                                   payload,
                                                   payload_size,
                                                   output,
                                                   openflash::max_packet_size,
                                                   encoded_size);

            if (!result)
                return;

            sequence_number++;

            for (size_t i = 0; i < encoded_size; i++)
                openflash::gba::uart_send(output[i]);
        }

        bool receive_packet(protocol &packet)
        {
            if (openflash::gba::uart_receive(received_byte))
                return parser.push_packet(received_byte, packet);
            return false;
        }

        void append_u8(uint8_t *payload, uint8_t value)
        {
            if (!payload)
                return;

            payload[0] = value;
        }

        void append_u16(uint8_t *payload, uint16_t value)
        {
            if (!payload)
                return;

            payload[0] = static_cast<uint8_t>(value & 0xFF);
            payload[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        }

        void append_string(uint8_t *payload, const bn::string<max_file_name_character_count> &value)
        {
            if (!payload)
                return;

            const uint16_t length = static_cast<uint16_t>(value.size());

            append_u16(payload, length);

            for (uint16_t i = 0; i < length; i++)
                payload[2 + i] = static_cast<uint8_t>(value[i]);
        }

        bool read_u16(const uint8_t *payload, size_t &offset, uint16_t &value)
        {
            if (!payload)
                return false;

            if (offset > max_payload_size - 2)
                return false;

            value = static_cast<uint16_t>(payload[offset]) | (static_cast<uint16_t>(payload[offset + 1]) << 8);

            offset += 2;

            return true;
        }

        bool read_string(const uint8_t *payload, size_t &offset, bn::string<max_file_name_character_count> &value)
        {
            uint16_t length = 0;

            if (!read_u16(payload, offset, length))
                return false;

            if (length > max_file_name_character_count)
                return false;

            if (offset > (size_t)(max_payload_size - length))
                return false;

            value.clear();

            for (uint16_t i = 0; i < length; i++)
                value.push_back(static_cast<char>(payload[offset + i]));

            offset += length;

            return true;
        }

        void append_u32(uint8_t *payload, uint32_t value)
        {
            if (!payload)
                return;

            payload[0] = static_cast<uint8_t>(value & 0xFF);
            payload[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
            payload[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
            payload[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
        }

        bool read_u32(const uint8_t *payload, size_t &offset, uint32_t &value)
        {
            if (!payload)
                return false;

            if (offset > max_payload_size - 4)
                return false;

            value = static_cast<uint32_t>(payload[offset]) | (static_cast<uint32_t>(payload[offset + 1]) << 8)
                    | (static_cast<uint32_t>(payload[offset + 2]) << 16)
                    | (static_cast<uint32_t>(payload[offset + 3]) << 24);

            offset += 4;

            return true;
        }
    }
}