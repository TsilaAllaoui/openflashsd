#include <Arduino.h>

#include "src/drivers/sd_filesystem.h"
#include "src/packet_stream_parser.h"
#include "src/request_handler.h"

using namespace openflash::esp32;

packet_stream_parser parser;
sd_filesystem filesystem;
request_handler handler(filesystem);

void setup()
{
    Serial.begin(115200);
    filesystem.begin();
}

void loop()
{
    while (Serial.available() > 0)
    {
        uint8_t byte = static_cast<uint8_t>(Serial.read());
        auto request = parser.push_packet(byte);

        if (!request)
            continue;

        auto responses = handler.handle(*request);

        for (const auto &response : responses)
        {
            auto bytes = serialize(response);

            if (!bytes.empty())
                Serial.write(bytes.data(), bytes.size());
        }
    }
}
