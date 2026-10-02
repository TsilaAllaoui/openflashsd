#ifndef GBA_PACKET_STREAM_PARSER_H
#define GBA_PACKET_STREAM_PARSER_H

#include "bn_vector.h"
#include "bn_optional.h"

#include <cstdint>
#include "protocol.h"

namespace openflash
{
    namespace gba
    {
        class packet_stream_parser
        {
          private:
            bn::vector<uint8_t, max_packet_size> _buffer;

          public:
            packet_stream_parser();
            void reset();
            bn::optional<protocol> push_packet(const uint8_t &byte);
            bool has_pending_data() const;
        };
    } // namespace gba
} // namespace openflash

#endif // GBA_PACKET_STREAM_PARSER_H