#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <vector>
#include <string>
#include <cstdint>
#include <optional>

/*
Byte(s)     Meaning
-------------------------------------------
0-1         Magic = "OF"
2           Protocol version
3           Sequence number
4           Command
5-6         Payload size
7...        Payload
last 2      CRC16
*/

namespace openflash
{
    namespace esp32
    {

        constexpr uint8_t magic_0 = 'O';
        constexpr uint8_t magic_1 = 'F';
        constexpr size_t header_size = 7;
        constexpr size_t crc_size = 2;
        constexpr size_t minimum_packet_size = header_size + crc_size;
        constexpr size_t max_payload_size = 1024;
        constexpr uint8_t current_protocol_version = 1;

        enum class command : uint8_t
        {
            PING = 0x01,
            GET_CART_INFO = 0x02,
            LIST_FILES = 0x03,
            GET_ROM_INFO = 0x04,
            GET_SAVE_INFO = 0x05,
            START_PROCESS = 0x06,
            GET_PROCESS_INFO = 0x07,
            RESET_PROCESS = 0x08
        };

        enum class status : uint8_t
        {
            OK = 0x00,
            ERROR = 0x01,
            INVALID_COMMAND = 0x02,
            INVALID_PAYLOAD = 0x03,
            NOT_FOUND = 0x04,
            NOT_READY = 0x05
        };

        struct protocol
        {
            uint8_t protocol_version = current_protocol_version;
            uint8_t sequence_number = 0;
            command cmd = command::PING;
            std::vector<uint8_t> payload;
        };

        bool is_command_supported(const command &command_);
        uint16_t calculate_crc16(const uint8_t *data, size_t size);
        std::string status_to_string(const status &status_);
        std::optional<protocol> deserialize(const std::vector<uint8_t> &bytes);
        std::vector<uint8_t> serialize(const protocol &protocol_);
    }
}

#endif // PROTOCOL_H