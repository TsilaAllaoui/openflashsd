#include "crc16.h"
#include "packet_encoder.h"

namespace openflash
{
    bool encode_packet(uint8_t sequence,
                       command command_,
                       const uint8_t *payload,
                       uint16_t payload_size,
                       uint8_t *output,
                       size_t output_capacity,
                       size_t &output_size)
    {
        if (!payload && command_ != command::PING && payload_size > 0)
            return false;

        size_t required_size = 9 + payload_size;

        if (output_capacity < required_size)
            return false;

        output_size = 0;

        output[output_size++] = magic_0;
        output[output_size++] = magic_1;
        output[output_size++] = current_protocol_version;
        output[output_size++] = sequence;
        output[output_size++] = static_cast<uint8_t>(command_);

        output[output_size++] = payload_size & 0xFF;
        output[output_size++] = payload_size >> 8;

        for (int i = 0; i < payload_size; i++)
            output[output_size++] = payload[i];

        auto crc16 = calculate_crc16(output, output_size);

        output[output_size++] = crc16 & 0xFF;
        output[output_size++] = crc16 >> 8;

        return true;
    }
}