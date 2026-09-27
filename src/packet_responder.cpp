#include "protocol.h"
#include "request_handler.h"

#include <iostream>

namespace openflash
{
    namespace esp32
    {
        protocol request_handler::handle(const protocol &request)
        {
            protocol response;

            response.protocol_version = current_protocol_version;
            response.sequence_number = request.sequence_number;
            response.cmd = request.cmd;

            switch (request.cmd)
            {
                case command::PING:
                {
                    if (!request.payload.empty())
                    {
                        response.payload.emplace_back(static_cast<uint8_t>(status::INVALID_PAYLOAD));
                    }
                    else
                    {
                        response.payload.emplace_back(static_cast<uint8_t>(status::OK));
                    }
                    break;
                }

                default:
                {
                    response.payload.emplace_back(static_cast<uint8_t>(status::INVALID_COMMAND));
                    break;
                }
            }

            return response;
        }
    }
}