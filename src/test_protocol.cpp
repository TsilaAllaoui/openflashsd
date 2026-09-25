#include <iostream>

#include "packet_manager.h"
#include "packet_responder.h"

int main()
{
    std::cout << "---------- TEST PROTOCOL ----------: " << std::endl;
    openflash::esp32::packet_manager packet_manager;
    openflash::esp32::packet_responder packet_responder;

    std::vector<openflash::esp32::command> commands = {
        openflash::esp32::command::PING,
        openflash::esp32::command::PING,
        //  openflash::esp32::command::GET_ROM_INFO,
        //  openflash::esp32::command::GET_CART_INFO,
        //  openflash::esp32::command::GET_SAVE_INFO
    };

    std::cout << "CRC corruption correctly detected" << std::endl;
    int count = 1;
    for (const auto &cmd : commands)
    {
        std::cout << "----------- Packet n°" << count << " -----------" << std::endl;
        std::cout << "Sending packet! " << std::endl;
        auto ping = packet_manager.send(cmd);
        std::cout << "Packet Sent! " << std::endl;

        auto request = openflash::esp32::deserialize(ping);
        if (!request)
        {
            std::cout << "Error deserializing request!\n";
            return 1;
        }
        auto request_sequence_number = request->sequence_number;

        std::cout << "Receiving packet" << std::endl;

        // corrupting bytes
        if (count++ == 2)
        {
            ping[3] ^= 0x01;
        }

        auto pong = packet_responder.receive(ping);
        std::cout << "Packet received" << std::endl;

        if (pong.empty())
        {
            std::cout << "Packet rejected - no response" << std::endl;
            continue;
        }

        // checking response deserialization
        auto response = openflash::esp32::deserialize(pong);

        if (!response)
        {
            std::cout << "Invalid response packet" << std::endl;
            return 1;
        }

        if (response->payload.empty())
        {
            std::cout << "Response has no status" << std::endl;
            return 1;
        }

        auto response_sequence_number = response->sequence_number;

        if (request_sequence_number != response_sequence_number)
        {
            std::cout << "Request sequence number and response sequence number diverged: " << request_sequence_number
                      << " != " << response_sequence_number << std::endl;
        }

        auto response_status = static_cast<openflash::esp32::status>(response->payload[0]);

        std::cout << "Response status: \"" << openflash::esp32::status_to_string(response_status) << "\"" << std::endl;
    }

    std::cout << "---------- END ----------: " << std::endl;
}