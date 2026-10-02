#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include <vector>

#include "protocol.h"
#include "drivers/i_filesystem.h"

namespace openflash
{
    namespace esp32
    {
        class request_handler
        {
        private:
            i_filesystem &_filesystem;

        public:
            request_handler(i_filesystem &filesystem);
            std::vector<protocol> handle(const protocol &request);
            void handle_ping(std::vector<uint8_t> &payload);
            void handle_get_cart_infos(std::vector<uint8_t> &payload);
            void handle_list_files(const protocol &request, std::vector<protocol> &responses);
            void handle_debug(const protocol &request, std::vector<uint8_t> &response);
        };
    }
}

#endif // PACKET_RESPONDER_H