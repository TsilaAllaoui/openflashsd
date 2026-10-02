#ifndef PACKET_STREAM_PARSER_H
#define PACKET_STREAM_PARSER_H

#include <cstdint>
#include <optional>
#include <vector>

#include "protocol.h"

namespace openflash
{
    namespace esp32
    {
        struct parser_stats
        {
            uint32_t garbage_bytes = 0;
            uint32_t invalid_magic = 0;
            uint32_t invalid_version = 0;
            uint32_t invalid_length = 0;
            uint32_t crc_errors = 0;
            uint32_t valid_packets = 0;
        };

        class packet_stream_parser
        {
        private:
            std::vector<uint8_t> _buffer;
            parser_stats _stats;

        public:
            packet_stream_parser();
            void reset();
            std::optional<protocol> push_packet(const uint8_t &byte);
            const parser_stats &stats() const;
        };
    }
}

#endif // PACKET_STREAM_PARSER_H