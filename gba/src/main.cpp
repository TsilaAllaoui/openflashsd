#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_optional.h"

#include "common.h"
#include "packet_encoder.h"
#include "utilities/pop_up.h"
#include "scene_state_machine.h"
#include "communication/protocol.h"
#include "communication/gba_uart.h"
#include "communication/packet_stream_parser.h"

static uint8_t sequence_number = 0;

void send_packet(const uint8_t *payload, uint16_t payload_size, uint8_t *output, openflash::command cmd)
{
    openflash::gba::protocol packet;
    packet.sequence_number = sequence_number;
    packet.cmd = cmd;

    size_t encoded_size = 0;
    bool result = openflash::encode_packet(packet.sequence_number,
                                           packet.cmd,
                                           payload,
                                           payload_size,
                                           output,
                                           openflash::max_packet_size,
                                           encoded_size);

    if (!result)
        return;

    sequence_number++;

    for (size_t i = 0; i < encoded_size; i++)
        openflash::gba::uart_send(output[i]);
}

int main()
{
    bn::core::init();
    openflash::gba::uart_init();

    openflash::scene_state_machine::instance().request_scene_state(openflash::scene_type::MAIN_MENU);

    bn::optional<openflash::pop_up> popup;
    openflash::gba::packet_stream_parser parser;

    while (true)
    {
        openflash::scene_state_machine::instance().update_current_scene();
        openflash::scene_state_machine::instance().render_current_scene();

        if (bn::keypad::start_pressed())
        {
            uint8_t packet_buffer[openflash::max_packet_size];
            send_packet(nullptr, 0, packet_buffer, openflash::command::PING);
        }

        if (bn::keypad::select_pressed())
        {
            uint8_t packet_buffer[openflash::max_packet_size];
            bn::string<256> message = "This is a debug text from gba tha say hello to esp32...";
            send_packet(reinterpret_cast<uint8_t *>(message.data()),
                        message.size(),
                        packet_buffer,
                        openflash::command::DEBUG);
        }

        uint8_t received_byte = 0;

        if (openflash::gba::uart_receive(received_byte))
        {
            auto result = parser.push_packet(received_byte);

            if (!result)
                continue;

            if (result->cmd == openflash::command::PING && !popup)
                popup.emplace("Received PONG from ESP32", true);
        }

        if (popup)
        {
            popup->update();

            if (popup->get_confirmation_response() == openflash::confirmation_request_status::NEGATIVE)
                popup.reset();
        }

        bn::core::update();
    }
}
