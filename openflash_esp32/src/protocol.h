#ifndef ESP32_PROTOCOL_H
#define ESP32_PROTOCOL_H

#include <string>
#include <vector>
#include <optional>

#include "common.h"

namespace openflash
{
    namespace esp32
    {
        struct protocol
        {
            uint8_t protocol_version = current_protocol_version;
            uint8_t sequence_number = 0;
            command cmd = command::PING;
            std::vector<uint8_t> payload;
        };

        bool is_command_supported(const command &command_);
        std::string status_to_string(const status &status_);
        std::optional<protocol> deserialize(const std::vector<uint8_t> &bytes);
        std::vector<uint8_t> serialize(const protocol &protocol_);
    }
}

#endif // ESP32_PROTOCOL_H
