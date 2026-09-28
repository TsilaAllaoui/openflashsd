#include <vector>
#include <iostream>

#include "protocol.h"

namespace openflash
{
    namespace esp32
    {
        Config config;

        bool is_command_supported(const command &command_)
        {
            switch (command_)
            {
                case command::PING:
                case command::GET_CART_INFO:
                case command::GET_PROCESS_INFO:
                case command::GET_ROM_INFO:
                case command::GET_SAVE_INFO:
                case command::LIST_FILES:
                case command::RESET_PROCESS:
                case command::START_PROCESS:
                    return true;

                default:
                    return false;
            }
        }

        std::optional<protocol> deserialize(const std::vector<uint8_t> &bytes)
        {
            //size checks
            if (bytes.size() < minimum_packet_size)
                return std::nullopt;

            if (bytes[0] != magic_0 || bytes[1] != magic_1)
                return std::nullopt;

            if (bytes[2] != current_protocol_version)
                return std::nullopt;

            uint16_t payload_size = static_cast<uint16_t>(bytes[5]) | (static_cast<uint16_t>(bytes[6]) << 8);

            if (payload_size > config.max_payload_size)
                return std::nullopt;

            size_t expected_size = header_size + payload_size + crc_size;

            if (bytes.size() != expected_size)
                return std::nullopt;

            // CRC16 checks
            uint16_t received_crc = static_cast<uint16_t>(bytes[bytes.size() - 2])
                                    | (static_cast<uint16_t>(bytes[bytes.size() - 1]) << 8);

            uint16_t calculated_crc = calculate_crc16(bytes.data(), bytes.size() - crc_size);

            if (received_crc != calculated_crc)
                return std::nullopt;

            // protocol
            protocol result;

            result.protocol_version = bytes[2];
            result.sequence_number = bytes[3];
            result.cmd = static_cast<command>(bytes[4]);

            result.payload.assign(bytes.begin() + header_size, bytes.begin() + header_size + payload_size);

            return result;
        }

        std::vector<uint8_t> serialize(const protocol &protocol_)
        {
            // limit serialization load
            if (protocol_.payload.size() > config.max_payload_size)
                return {};

            // only ping for now
            std::vector<uint8_t> packet;

            // magic
            packet.emplace_back(magic_0);
            packet.emplace_back(magic_1);

            // protocol version
            packet.emplace_back(protocol_.protocol_version);

            // sequence number
            packet.emplace_back(protocol_.sequence_number);

            // command
            packet.emplace_back(static_cast<uint8_t>(protocol_.cmd));

            // payload
            packet.emplace_back(protocol_.payload.size() & 0xFF);
            packet.emplace_back(protocol_.payload.size() >> 8);
            for (const auto &byte : protocol_.payload)
                packet.emplace_back(byte);

            // CRC16
            auto crc16 = calculate_crc16(packet.data(), packet.size());
            packet.emplace_back(crc16 & 0xFF);
            packet.emplace_back((crc16 >> 8) & 0xFF);

            // minimal bytes in packet check (should be 5 at least)
            if (packet.size() < minimum_packet_size)
                return {};

            return packet;
        }

        uint16_t calculate_crc16(const uint8_t *data, size_t size)
        {
            uint16_t crc = 0xFFFF;

            for (size_t index = 0; index < size; ++index)
            {
                crc ^= static_cast<uint16_t>(data[index]) << 8;

                for (int bit = 0; bit < 8; ++bit)
                {
                    if (crc & 0x8000)
                        crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
                    else
                        crc <<= 1;
                }
            }

            return crc;
        }

        std::string status_to_string(const status &status_)
        {
            switch (status_)
            {
                case status::ERROR:
                    return "Error occured";
                case status::INVALID_COMMAND:
                    return "Invalid command";
                case status::INVALID_PAYLOAD:
                    return "Invalid payload";
                case status::NOT_FOUND:
                    return "Not found";
                case status::NOT_READY:
                    return "Not ready";
                case status::OK:
                    return "OK";
                case status::NOT_FINISHED_YET:
                    return "Not finished yet";

                default:
                    return "Unhandled status";
            }
        }
    }
}