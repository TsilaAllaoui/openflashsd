#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include "protocol.h"

namespace openflash
{
    namespace esp32
    {
        class request_handler
        {
        public:
            protocol handle(const protocol &request);
        };
    }
}

#endif // PACKET_RESPONDER_H