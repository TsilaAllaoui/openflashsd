#include "crc16.h"
#include "communication/packet_decoder.h"

namespace openflash
{
    namespace gba
    {
        bool decode_packet(const uint8_t * bytes, const size_t& size, protocol & protocol_)
        {
            if (!bytes)
                return false;

            if (size < minimum_packet_size)
                return false;

            if (bytes[0] != magic_0 || bytes[1] != magic_1)
                return false;

            if (bytes[2] != current_protocol_version)
                return false;

            uint16_t payload_size = static_cast<uint16_t>(bytes[5]) | (static_cast<uint16_t>(bytes[6]) << 8);
            if (payload_size > max_payload_size)
                return false;

            size_t expected_size = header_size + payload_size + crc_size;
            if (size != expected_size)
                return false;

            auto crc16 = calculate_crc16(bytes, size - crc_size);
            auto packet_crc16 = (static_cast<uint16_t>(bytes[size - 2])
                                 | (static_cast<uint16_t>(bytes[size - 1]) << 8));
            if (crc16 != packet_crc16)
                return false;

            protocol_.sequence_number = bytes[3];
            protocol_.cmd = static_cast<command>(bytes[4]);
            protocol_.payload.clear();

            for (size_t i = 0; i < payload_size; i++)
                protocol_.payload.push_back(bytes[header_size + i]);

            return true;
        }
    }
}

