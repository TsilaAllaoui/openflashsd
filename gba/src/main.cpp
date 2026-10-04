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

using namespace openflash::gba;

int main()
{
    bn::core::init();
    uart_init();

    scene_state_machine::instance().request_scene_state(scene_type::MAIN_MENU);

    while (true)
    {
        scene_state_machine::instance().update_current_scene();
        scene_state_machine::instance().render_current_scene();
        bn::core::update();
    }
}
