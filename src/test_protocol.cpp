#include <vector>
#include <random>
#include <cstdint>
#include <sstream>
#include <optional>
#include <iostream>

#include "protocol.h"
#include "request_sender.h"
#include "request_handler.h"
#include "packet_stream_parser.h"

using namespace openflash::esp32;

namespace
{
    void print_stats(const parser_stats &stats)
    {
        std::stringstream ss;
        ss << std::endl;
        ss << "Garbage bytes:         " << stats.garbage_bytes << std::endl;
        ss << "Invalid versions:          " << stats.invalid_version << std::endl;
        ss << "Invalid lengths:           " << stats.invalid_length << std::endl;
        ss << "CRC failures:              " << stats.crc_errors << std::endl;
        ss << "Valid packets:          " << stats.valid_packets << std::endl;
        std::cout << ss.str() << std::endl;
    }

    bool check_packet(const protocol &packet, command expected_command, uint8_t expected_sequence)
    {
        if (packet.cmd != expected_command)
        {
            std::cout << "Wrong command" << std::endl;
            return false;
        }

        if (packet.sequence_number != expected_sequence)
        {
            std::cout << "Wrong sequence" << std::endl;
            return false;
        }

        return true;
    }

    std::optional<protocol> feed(packet_stream_parser &parser, const std::vector<uint8_t> &bytes)
    {
        std::optional<protocol> result;

        for (uint8_t byte : bytes)
        {
            auto packet = parser.push_packet(byte);

            if (packet)
            {
                if (result)
                {
                    std::cout << "Unexpected second packet" << std::endl;
                    return std::nullopt;
                }

                result = packet;
            }
        }

        return result;
    }

    bool test_normal_packet()
    {
        std::cout << "[TEST] Normal packet" << std::endl;

        request_sender sender;
        packet_stream_parser parser;

        auto bytes = sender.send(command::PING);
        auto expected = deserialize(bytes);

        if (!expected)
            return false;

        auto received = feed(parser, bytes);

        if (!received)
            return false;

        return check_packet(*received, command::PING, expected->sequence_number);
    }

    bool test_garbage_before_packet()
    {
        std::cout << "[TEST] Garbage before packet" << std::endl;

        request_sender sender;
        packet_stream_parser parser;

        std::vector<uint8_t> stream = {0x12, 0x77, 0xAB, 0x91, 0x00};

        auto packet_bytes = sender.send(command::PING);
        auto expected = deserialize(packet_bytes);

        if (!expected)
            return false;

        stream.insert(stream.end(), packet_bytes.begin(), packet_bytes.end());

        auto received = feed(parser, stream);

        if (!received)
            return false;

        return check_packet(*received, command::PING, expected->sequence_number);
    }

    bool test_corrupted_packet()
    {
        std::cout << "[TEST] CRC corruption" << std::endl;

        request_sender sender;
        packet_stream_parser parser;

        auto bytes = sender.send(command::PING);

        if (bytes.size() < 5)
            return false;

        bytes[3] ^= 0x40;

        auto received = feed(parser, bytes);

        if (received)
        {
            std::cout << "Corrupted packet was accepted" << std::endl;
            return false;
        }

        return true;
    }

    bool test_incomplete_packet()
    {
        std::cout << "[TEST] Incomplete packet" << std::endl;

        request_sender sender;
        packet_stream_parser parser;

        auto bytes = sender.send(command::PING);

        if (bytes.size() < 4)
            return false;

        std::optional<protocol> result;

        for (size_t index = 0; index < 4; ++index)
        {
            result = parser.push_packet(bytes[index]);

            if (result)
            {
                std::cout << "Parser returned packet too early" << std::endl;
                return false;
            }
        }

        for (size_t index = 4; index < bytes.size(); ++index)
        {
            auto packet = parser.push_packet(bytes[index]);

            if (packet)
                result = packet;
        }

        return result.has_value();
    }

    bool test_back_to_back_packets()
    {
        std::cout << "[TEST] Back-to-back packets" << std::endl;

        request_sender sender;
        packet_stream_parser parser;

        auto packet1 = sender.send(command::PING);
        auto packet2 = sender.send(command::PING);

        auto expected1 = deserialize(packet1);
        auto expected2 = deserialize(packet2);

        if (!expected1 || !expected2)
            return false;

        std::vector<uint8_t> stream;

        stream.insert(stream.end(), packet1.begin(), packet1.end());

        stream.insert(stream.end(), packet2.begin(), packet2.end());

        std::vector<protocol> received_packets;

        for (uint8_t byte : stream)
        {
            auto packet = parser.push_packet(byte);

            if (packet)
                received_packets.emplace_back(*packet);
        }

        if (received_packets.size() != 2)
        {
            std::cout << "Expected 2 packets, received " << received_packets.size() << std::endl;

            return false;
        }

        return received_packets[0].sequence_number == expected1->sequence_number
               && received_packets[1].sequence_number == expected2->sequence_number;
    }

    bool test_random_stream()
    {
        std::cout << "[TEST] Random stream stress test" << std::endl;

        constexpr int iterations = 10000;

        request_sender sender;
        packet_stream_parser parser;

        std::mt19937 random(12345);
        std::uniform_int_distribution<int> random_byte(0, 255);
        std::uniform_int_distribution<int> action(0, 99);
        std::uniform_int_distribution<int> garbage_count(0, 20);

        int expected_valid_packets = 0;
        int received_valid_packets = 0;

        for (int iteration = 0; iteration < iterations; ++iteration)
        {
            int current_action = action(random);

            if (current_action < 60)
            {
                int count = garbage_count(random);

                for (int index = 0; index < count; ++index)
                {
                    uint8_t byte = static_cast<uint8_t>(random_byte(random));

                    auto packet = parser.push_packet(byte);

                    if (packet)
                    {
                        std::cout << "Unexpected packet inside random garbage" << std::endl;

                        return false;
                    }
                }
            }
            else if (current_action < 90)
            {
                auto bytes = sender.send(command::PING);

                expected_valid_packets++;

                for (uint8_t byte : bytes)
                {
                    auto packet = parser.push_packet(byte);

                    if (packet)
                    {
                        received_valid_packets++;

                        if (packet->cmd != command::PING)
                            return false;
                    }
                }
            }
            else
            {
                auto bytes = sender.send(command::PING);

                if (bytes.size() > 4)
                    bytes[3] ^= 0x80;

                for (uint8_t byte : bytes)
                {
                    auto packet = parser.push_packet(byte);

                    if (packet)
                    {
                        std::cout << "Corrupted packet accepted" << std::endl;

                        return false;
                    }
                }
            }
        }

        print_stats(parser.stats());

        if (received_valid_packets != expected_valid_packets)
        {
            std::cout << "Expected valid packets: " << expected_valid_packets << std::endl;

            std::cout << "Received valid packets: " << received_valid_packets << std::endl;

            return false;
        }

        std::cout << "Accepted " << received_valid_packets << " valid packets" << std::endl;

        return true;
    }
}

bool test_nested_resynchronization()
{
    request_sender sender;
    packet_stream_parser parser;

    auto real_packet = sender.send(command::PING);

    std::vector<uint8_t> stream = {magic_0,
                                   magic_1,
                                   current_protocol_version,
                                   0x55,
                                   static_cast<uint8_t>(command::PING),
                                   static_cast<uint8_t>(real_packet.size() & 0xFF),
                                   static_cast<uint8_t>((real_packet.size() >> 8) & 0xFF)};

    stream.insert(stream.end(), real_packet.begin(), real_packet.end());

    stream.emplace_back(0x00);
    stream.emplace_back(0x00);

    std::optional<protocol> recovered;

    for (uint8_t byte : stream)
    {
        auto packet = parser.push_packet(byte);

        if (packet)
            recovered = packet;
    }

    if (!recovered)
    {
        std::cout << "Failed to recover nested packet" << std::endl;

        return false;
    }

    if (recovered->cmd != command::PING)
    {
        std::cout << "Recovered wrong command" << std::endl;

        return false;
    }

    std::cout << "Nested packet recovered successfully" << std::endl;

    return true;
}

bool test_ping_round_trip()
{
    std::cout << "[TEST] Complete PING round trip" << std::endl;

    request_sender gba_manager;

    packet_stream_parser esp32_parser;
    request_handler esp32_handler;

    packet_stream_parser gba_parser;

    // gba creates PING.
    auto request_bytes = gba_manager.send(command::PING);

    if (request_bytes.empty())
        return false;

    // simulated gba -> esp32 wire
    std::optional<protocol> request;

    for (uint8_t byte : request_bytes)
    {
        auto packet = esp32_parser.push_packet(byte);

        if (packet)
            request = packet;
    }

    if (!request)
    {
        std::cout << "ESP32 did not receive request" << std::endl;
        return false;
    }
    // esp32 handles request
    protocol response = esp32_handler.handle(*request);

    // esp32 serializes response
    auto response_bytes = serialize(response);

    if (response_bytes.empty())
        return false;

    // simulated esp32 -> gba wire
    std::optional<protocol> received_response;

    for (uint8_t byte : response_bytes)
    {
        auto packet = gba_parser.push_packet(byte);

        if (packet)
            received_response = packet;
    }

    if (!received_response)
    {
        std::cout << "GBA did not receive response" << std::endl;
        return false;
    }

    // validate response
    if (received_response->sequence_number != request->sequence_number)
    {
        std::cout << "Sequence mismatch" << std::endl;
        return false;
    }

    if (received_response->cmd != command::PING)
    {
        std::cout << "Wrong response command" << std::endl;
        return false;
    }

    if (received_response->payload.size() != 1)
    {
        std::cout << "Wrong PING response payload" << std::endl;
        return false;
    }

    auto response_status = static_cast<status>(received_response->payload[0]);

    if (response_status != status::OK)
    {
        std::cout << "PING response is not OK" << std::endl;
        return false;
    }

    std::cout << "PING -> PONG OK, sequence=" << static_cast<int>(received_response->sequence_number) << std::endl;

    return true;
}

bool test_pings_round_trips_with_timeout()
{
    std::cout << "[TEST] Complete multiples PINGs round trips with timeout" << std::endl;

    request_sender gba_manager;

    packet_stream_parser esp32_parser;
    request_handler esp32_handler;

    packet_stream_parser gba_parser;

    // gba creates PING.
    std::vector<std::vector<uint8_t>> requests_bytes_list = {gba_manager.send(command::PING),
                                                             gba_manager.send(command::PING),
                                                             gba_manager.send(command::PING)};

    // suppose first request is sent and it succeed and got responded correctly
    // then second request got timeout so it fail and got ignored
    // then third request is sent and it succeed and got responded correctly
    for (int i = 0; i < requests_bytes_list.size(); i++)
    {
        auto request_bytes = requests_bytes_list[i];
        if (request_bytes.empty())
            return false;

        // simulated gba -> esp32 wire
        std::optional<protocol> request;

        int byte_counter = 0;
        for (uint8_t byte : request_bytes)
        {
            byte_counter++;

            // simulate timeout on second request 5th byte
            if (i == 1 && byte_counter == 4)
            {
                esp32_parser.reset();
            }
            else
            {
                auto packet = esp32_parser.push_packet(byte);

                if (packet)
                    request = packet;
            }
        }

        if (!request)
        {
            std::cout << "PING -> PONG KO: ESP32 did not receive request! Timeout occurent on request n°" << i + 1
                      << std::endl;
            continue;
        }

        // esp32 handles request
        protocol response = esp32_handler.handle(*request);

        // esp32 serializes response
        auto response_bytes = serialize(response);

        if (response_bytes.empty())
            return false;

        // simulated esp32 -> gba wire
        std::optional<protocol> received_response;

        for (uint8_t byte : response_bytes)
        {
            auto packet = gba_parser.push_packet(byte);

            if (packet)
                received_response = packet;
        }

        if (!received_response)
        {
            std::cout << "PING -> PONG KO: GBA did not receive response for request n°" << i + 1 << std::endl;
            continue;
        }

        // validate response
        if (received_response->sequence_number != request->sequence_number)
        {
            std::cout << "PING -> PONG KO: Sequence mismatch for request n°" << i + 1 << std::endl;
            continue;
        }

        if (received_response->cmd != command::PING)
        {
            std::cout << "PING -> PONG KO: Wrong response command for request n°" << i + 1 << std::endl;
            continue;
        }

        if (received_response->payload.size() != 1)
        {
            std::cout << "PING -> PONG KO: Wrong PING response payload for request n°" << i + 1 << std::endl;
            continue;
        }

        auto response_status = static_cast<status>(received_response->payload[0]);

        if (response_status != status::OK)
            std::cout << "PING -> PONG KO: PING response is not OK for request n°" << i + 1 << std::endl;

        std::cout << "PING -> PONG OK, sequence=" << static_cast<int>(received_response->sequence_number) << std::endl;
    }

    return true;
}

int main()
{
    std::cout << "========== OPENFLASH PROTOCOL TEST ==========" << std::endl;

    bool success = test_normal_packet() && test_garbage_before_packet() && test_corrupted_packet()
                   && test_incomplete_packet() && test_back_to_back_packets() && test_random_stream()
                   && test_nested_resynchronization() && test_ping_round_trip()
                   && test_pings_round_trips_with_timeout();

    std::cout << std::endl;

    if (!success)
    {
        std::cout << "========== TEST FAILED ==========" << std::endl;
        return 1;
    }

    std::cout << "========== ALL TESTS PASSED ==========" << std::endl;

    return 0;
}