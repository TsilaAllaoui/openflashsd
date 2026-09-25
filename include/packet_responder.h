#ifndef PACKET_RESPONDER_H
#define PACKET_RESPONDER_H

#include "protocol.h"

namespace openflash
{
    namespace esp32
    {
        class packet_responder
        {
        public:
            std::vector<uint8_t> receive(const std::vector<uint8_t> &bytes);
        };
    }
}

#endif // PACKET_RESPONDER_H