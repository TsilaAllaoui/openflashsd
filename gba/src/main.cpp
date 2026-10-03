#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_optional.h"

#include "common.h"
#include "packet_encoder.h"
#include "utilities/pop_up.h"
#include "scene_state_machine.h"
#include "communication/protocol.h"
#include "communication/gba_uart.h"
#include "communication/utilities.h"
#include "communication/packet_stream_parser.h"

int main()
{
    bn::core::init();
    openflash::gba::uart_init();

    openflash::scene_state_machine::instance().request_scene_state(openflash::scene_type::MAIN_MENU);

    bn::optional<openflash::pop_up> popup;

    while (true)
    {
        openflash::scene_state_machine::instance().update_current_scene();
        openflash::scene_state_machine::instance().render_current_scene();

        if (bn::keypad::start_pressed())
        {
            uint8_t packet_buffer[openflash::max_packet_size];
            openflash::gba::send_packet(nullptr, 0, packet_buffer, openflash::command::PING);
        }

        if (bn::keypad::select_pressed())
        {
            uint8_t packet_buffer[openflash::max_packet_size];
            bn::string<256> message = "This is a debug text from gba tha say hello to esp32...";
            openflash::gba::send_packet(reinterpret_cast<uint8_t *>(message.data()),
                                        message.size(),
                                        packet_buffer,
                                        openflash::command::DEBUG);
        }

        // drains all bytes received in one frame
        openflash::gba::protocol packet;
        while (openflash::gba::receive_packet(packet))
        {
            if (packet.cmd == openflash::command::PING && !popup)
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
