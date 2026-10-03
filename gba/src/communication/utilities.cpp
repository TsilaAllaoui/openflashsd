#include <cstdint>

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

        void send_packet(const uint8_t *payload, uint16_t payload_size, uint8_t *output, openflash::command cmd)
        {
            openflash::gba::protocol packet;
            packet.sequence_number = sequence_number;
            packet.cmd = cmd;

            size_t encoded_size = 0;
            bool result = openflash::encode_packet(packet.sequence_number,
                                                   packet.cmd,
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
    }
}