#include "protocol.h"
#include "config/config.h"
#include "packet_stream_parser.h"

namespace openflash
{
    namespace esp32
    {
        packet_stream_parser::packet_stream_parser()
        {
            _buffer.reserve(header_size + config.max_payload_size + crc_size);
        }

        void packet_stream_parser::reset()
        {
            _buffer.clear();
        }

        const parser_stats &packet_stream_parser::stats() const
        {
            return _stats;
        }

        std::optional<protocol> packet_stream_parser::push_packet(const uint8_t &byte)
        {
            _buffer.emplace_back(byte);

            while (true)
            {
                while (!_buffer.empty() && _buffer.front() != magic_0)
                {
                    _buffer.erase(_buffer.begin());
                    _stats.garbage_bytes++;
                }

                if (_buffer.empty())
                    return std::nullopt;

                if (_buffer.size() == 1)
                    return std::nullopt;

                if (_buffer[1] != magic_1)
                {
                    _buffer.erase(_buffer.begin());
                    _stats.invalid_magic++;
                    continue;
                }

                if (_buffer.size() < header_size)
                    return std::nullopt;

                if (_buffer[2] != current_protocol_version)
                {
                    _buffer.erase(_buffer.begin());
                    _stats.invalid_version++;
                    continue;
                }

                uint16_t payload_size = static_cast<uint16_t>(_buffer[5]) | (static_cast<uint16_t>(_buffer[6]) << 8);

                if (payload_size > config.max_payload_size)
                {
                    _buffer.erase(_buffer.begin());
                    _stats.invalid_length++;
                    continue;
                }

                size_t expected_size = header_size + payload_size + crc_size;

                if (_buffer.size() < expected_size)
                    return std::nullopt;

                std::vector<uint8_t> candidate(_buffer.begin(), _buffer.begin() + expected_size);

                auto result = deserialize(candidate);

                if (result)
                {
                    _buffer.erase(_buffer.begin(), _buffer.begin() + expected_size);

                    _stats.valid_packets++;

                    return result;
                }

                _buffer.erase(_buffer.begin());
                _stats.crc_errors++;
            }
        }
    }
}