#include "crc16.h"
#include "communication/packet_decoder.h"

namespace openflash
{
    namespace gba
    {
        bool decode_packet(const uint8_t * bytes, const size_t& size, protocol & protocol_)
        {
            if (bytes[0] != magic_0 || bytes[1] != magic_1)
                return false;

            if (bytes[2] != current_protocol_version)
                return false;

            protocol_.sequence_number = bytes[3];
            protocol_.cmd = static_cast<command>(bytes[4]);
            protocol_.payload.clear();

            bn::vector<uint8_t, max_packet_size> packet_bytes;
            for (size_t i = 0; i < size - 2; i++)
            {
                packet_bytes.emplace_back(bytes[i]);
                if (i >= header_size)
                    protocol_.payload.push_back(bytes[header_size + i]);
            }

            auto crc16 = calculate_crc16(packet_bytes.data(), packet_bytes.size());
            auto packet_crc16 = (static_cast<uint16_t>(bytes[size - 2]) | (static_cast<uint16_t>(bytes[size - 1]) << 8));
            if (crc16 != packet_crc16)
                return false;

            return true;
        }
    }
}

