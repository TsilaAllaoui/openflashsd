#include <vector>

#include "crc16.h"
#include "common.h"
#include "protocol.h"
#include "config/config.h"
#include "packet_encoder.h"

namespace openflash
{
    namespace esp32
    {
        std::optional<protocol> deserialize(const std::vector<uint8_t> &bytes)
        {
            //size checks
            if (bytes.size() < minimum_packet_size)
                return std::nullopt;

            if (bytes[0] != magic_0 || bytes[1] != magic_1)
                return std::nullopt;

            if (bytes[2] != current_protocol_version)
                return std::nullopt;

            uint16_t payload_size = static_cast<uint16_t>(bytes[5]) | (static_cast<uint16_t>(bytes[6]) << 8);

            if (payload_size > config.max_payload_size)
                return std::nullopt;

            size_t expected_size = header_size + payload_size + crc_size;

            if (bytes.size() != expected_size)
                return std::nullopt;

            // CRC16 checks
            uint16_t received_crc = static_cast<uint16_t>(bytes[bytes.size() - 2])
                                    | (static_cast<uint16_t>(bytes[bytes.size() - 1]) << 8);

            uint16_t calculated_crc = calculate_crc16(bytes.data(), bytes.size() - crc_size);

            if (received_crc != calculated_crc)
                return std::nullopt;

            // protocol
            protocol result;

            result.protocol_version = bytes[2];
            result.sequence_number = bytes[3];
            result.cmd = static_cast<command>(bytes[4]);

            result.payload.assign(bytes.begin() + header_size, bytes.begin() + header_size + payload_size);

            return result;
        }

        std::vector<uint8_t> serialize(const protocol &protocol_)
        {
            std::vector<uint8_t> result(9 + protocol_.payload.size());

            size_t encoded_size = 0;

            auto encoding_result = encode_packet(protocol_.sequence_number,
                                                 protocol_.cmd,
                                                 protocol_.payload.data(),
                                                 protocol_.payload.size(),
                                                 result.data(),
                                                 result.size(),
                                                 encoded_size);

            if (!encoding_result)
                return {};

            result.resize(encoded_size);

            return result;
        }


    }
}