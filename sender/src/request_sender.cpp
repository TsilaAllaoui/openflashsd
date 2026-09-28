#include <vector>
#include <iostream>

#include "request_sender.h"
#include "utilities.h"

namespace openflash
{
    namespace esp32
    {
        uint8_t request_sender::get_current_sequence_number()
        {
            static uint8_t current_sequence = 0;
            return current_sequence++;
        }

        std::vector<uint8_t> request_sender::send(command command_, const std::vector<uint8_t> &request)
        {
            std::vector<uint8_t> packet;

            protocol protocol_;

            auto sequence_number = get_current_sequence_number();

            // protocol version
            protocol_.protocol_version = 1;

            // sequence number
            protocol_.sequence_number = sequence_number;

            // command
            protocol_.cmd = command_;

            if (command_ == command::PING)
            {
                packet = serialize(protocol_);
            }
            else if (command_ == command::LIST_FILES)
            {
                std::string requested_path;
                size_t offset = 0;
                read_string(request, offset, requested_path);
                append_string(protocol_.payload, requested_path);
                packet = serialize(protocol_);
            }

            return packet;
        }
    }
}