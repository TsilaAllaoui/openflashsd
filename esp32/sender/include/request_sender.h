#ifndef PACKET_REQUEST_SENDER_H
#define PACKET_REQUEST_SENDER_H

#include "protocol.h"

namespace openflash
{
    namespace esp32
    {
        class request_sender
        {
        private:
            uint8_t get_current_sequence_number();

        public:
            std::vector<uint8_t> send(command command_, const std::vector<uint8_t> &request = {});
        };
    }
}

#endif // PACKET_MANAGER_H
