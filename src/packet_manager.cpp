#include <vector>
#include <iostream>

#include "packet_manager.h"

namespace openflash
{
    namespace esp32
    {
        uint8_t packet_manager::get_current_sequence_number()
        {
            static uint8_t current_sequence = 0;
            return current_sequence++;
        }

        std::vector<uint8_t> packet_manager::send(command command_)
        {
            std::vector<uint8_t> packet;

            if (command_ == command::PING)
            {
                protocol protocol_;

                auto sequence_number = get_current_sequence_number();

                // protocol version
                protocol_.protocol_version = 1;

                // sequence number
                protocol_.sequence_number = sequence_number;

                // command
                protocol_.cmd = command_;

                packet = serialize(protocol_);
                // call gba here and send command
            }

            return packet;
        }
    }
}