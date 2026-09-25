#ifndef PACKET_MANAGER_H
#define PACKET_MANAGER_H

#include "protocol.h"

namespace openflash
{
    namespace esp32
    {
        class packet_manager
        {
        private:
            uint8_t get_current_sequence_number();

        public:
            std::vector<uint8_t> send(command command_);
        };
    }
}

#endif // PACKET_MANAGER_H
