#include "protocol.h"
#include "packet_responder.h"

#include <iostream>

namespace openflash
{
    namespace esp32
    {
        std::vector<uint8_t> packet_responder::receive(const std::vector<uint8_t> &bytes)
        {
            std::vector<uint8_t> response;

            auto parsed_protocol = deserialize(bytes);

            if (!parsed_protocol)
            {
                std::cout << "Log: Error parsing bytes for protocol!\n";
                return {};
            }
            else
            {
                if (parsed_protocol->cmd == command::PING)
                {
                    parsed_protocol->payload.emplace(parsed_protocol->payload.begin(),
                                                     static_cast<uint8_t>(status::OK));
                }
                // do other types here
            }

            return serialize(*parsed_protocol);
        }
    }
}