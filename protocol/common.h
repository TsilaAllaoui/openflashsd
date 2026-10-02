#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <cstddef>

namespace openflash
{
    static constexpr uint8_t magic_0 = 'O';
    static constexpr uint8_t magic_1 = 'F';

    static constexpr uint8_t current_protocol_version = 1;
    constexpr size_t header_size = 7;
    constexpr size_t crc_size = 2;
    constexpr size_t minimum_packet_size = header_size + crc_size;

    static constexpr uint16_t max_payload_size = 4096;
    constexpr size_t max_packet_size = minimum_packet_size + max_payload_size;

    enum class command : uint8_t
    {
        PING = 0x01,
        GET_CART_INFO = 0x02,
        LIST_FILES = 0x03,
        GET_ROM_INFO = 0x04,
        GET_SAVE_INFO = 0x05,
        START_PROCESS = 0x06,
        GET_PROCESS_INFO = 0x07,
        RESET_PROCESS = 0x08,
        DEBUG = 0x09
    };

    enum class status : uint8_t
    {
        OK = 0x00,
        ERROR = 0x01,
        INVALID_COMMAND = 0x02,
        INVALID_PAYLOAD = 0x03,
        NOT_FOUND = 0x04,
        NOT_READY = 0x05,
        NOT_FINISHED_YET = 0x06,
        DEBUG = 0x07
    };

    enum class file_type : uint8_t
    {
        NORMAL_FILE = 0,
        FOLDER = 1,
        GBA_FILE = 2,
        SAVE_FILE = 3
    };
}

#endif // COMMON_H