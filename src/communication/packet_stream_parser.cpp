#include <algorithm>

#include "protocol.h"
#include "packet_decoder.h"
#include "communication/packet_stream_parser.h"

namespace openflash
{
    namespace gba
    {
        packet_stream_parser::packet_stream_parser()
        {
        }

        void packet_stream_parser::reset()
        {
            _buffer.clear();
        }

        bool packet_stream_parser::has_pending_data() const
        {
            return !_buffer.empty();
        }

        bn::optional<protocol> packet_stream_parser::push_packet(const uint8_t &byte)
        {
            _buffer.emplace_back(byte);

            while (true)
            {
                while (!_buffer.empty() && _buffer.front() != magic_0)
                {
                    _buffer.erase(_buffer.begin());
                }

                if (_buffer.empty())
                    return bn::nullopt;

                if (_buffer.size() == 1)
                    return bn::nullopt;

                if (_buffer[1] != magic_1)
                {
                    _buffer.erase(_buffer.begin());
                    continue;
                }

                if (static_cast<size_t>(_buffer.size()) < header_size)
                    return bn::nullopt;

                if (_buffer[2] != current_protocol_version)
                {
                    _buffer.erase(_buffer.begin());
                    continue;
                }

                uint16_t payload_size = static_cast<uint16_t>(_buffer[5]) | (static_cast<uint16_t>(_buffer[6]) << 8);

                if (payload_size > max_payload_size)
                {
                    _buffer.erase(_buffer.begin());
                    continue;
                }

                size_t expected_size = header_size + payload_size + crc_size;

                if (static_cast<size_t>(_buffer.size()) < expected_size)
                    return bn::nullopt;

                protocol protocol_;
                auto result = decode_packet(_buffer.data(), expected_size, protocol_);

                if (result)
                {
                    _buffer.erase(_buffer.begin(), _buffer.begin() + expected_size);
                    _buffer.clear();
                    return protocol_;
                }

                _buffer.erase(_buffer.begin());
                return bn::nullopt;
            }
        }
    }
}