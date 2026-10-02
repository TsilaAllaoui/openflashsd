#include <Arduino.h>

#include "common.h"
#include "src/constants.h"
#include "src/request_handler.h"
#include "src/packet_stream_parser.h"
#include "src/drivers/sd_filesystem.h"

using namespace openflash::esp32;

packet_stream_parser parser;
sd_filesystem filesystem;
request_handler handler(filesystem);

void printProtocol(const protocol &p)
{
    Serial.println(F("========================================"));
    Serial.println(F("           PROTOCOL PACKET              "));
    Serial.println(F("========================================"));

    // Header Info
    Serial.print(F("Protocol Version : "));
    Serial.println(p.protocol_version);
    Serial.print(F("Sequence Number  : "));
    Serial.println(p.sequence_number);
    Serial.print(F("Command (CMD)    : 0x"));
    if (static_cast<uint8_t>(p.cmd) < 16)
        Serial.print('0');
    Serial.println(static_cast<uint8_t>(p.cmd), HEX);

    // Payload Details
    Serial.print(F("Payload Size     : "));
    Serial.print(p.payload.size());
    Serial.println(F(" bytes"));
    Serial.println(F("Payload Data     :"));

    if (p.payload.empty())
    {
        Serial.println(F("  [ Empty ]"));
    }
    else
    {
        Serial.print(F("  "));
        for (size_t i = 0; i < p.payload.size(); ++i)
        {
            // Print hex byte
            Serial.print(F("0x"));
            if (p.payload[i] < 16)
                Serial.print('0');
            Serial.print(p.payload[i], HEX);

            // Format spacing: 8 bytes per line or space-separated
            if (i < p.payload.size() - 1)
            {
                if ((i + 1) % 8 == 0)
                {
                    Serial.print(F("\n  ")); // New line every 8 bytes with indentation
                }
                else
                {
                    Serial.print(F(" ")); // Space between bytes
                }
            }
        }
        Serial.println();
    }
    Serial.println(F("========================================\n"));
}

void setup()
{
    Serial.begin(115200); // USB debug console

    Serial1.begin(115200, SERIAL_8N1, PIN_SO, PIN_SI); // GBA communication

    if (!filesystem.begin())
    {
        Serial.println("Failed to initialize SD card");
        while (true)
            ;
    }
}

void loop()
{
    while (Serial1.available() > 0)
    {
        uint8_t byte = static_cast<uint8_t>(Serial1.read());

        auto request = parser.push_packet(byte);

        if (!request)
            continue;

        Serial.println("Received request:");
        printProtocol(*request);

        auto responses = handler.handle(*request);

        Serial.println("Request handled correclty with " + String(responses.size()) + " responses!");

        for (const auto &response : responses)
        {
            if (response.cmd == openflash::command::DEBUG)
            {
                Serial.println("GBA DEBUG: ");
                for (size_t i = 0; i < response.payload.size(); ++i)
                {
                    Serial.print(static_cast<char>(response.payload[i]));
                }
                Serial.println();
                continue;
            }

            auto bytes = serialize(response);

            if (!bytes.empty())
            {
                Serial.println("Response sent to GBA: ");
                printProtocol(response);
                Serial1.write(bytes.data(), bytes.size());
            }
        }
    }
}
