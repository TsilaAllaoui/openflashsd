#ifndef DUMP_ROM_INFO_SCENE_H
#define DUMP_ROM_INFO_SCENE_H

#include "bn_display.h"
#include "bn_sprite_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_text_generator.h"

#include "constants.h"
#include "cart_infos.h"
#include "api/requests.h"
#include "scenes/i_scene.h"
#include "utilities/pop_up.h"
#include "scenes/scene_type.h"

namespace openflash
{
    namespace gba
    {
        class dump_rom_info_scene : public i_scene
        {
        private:
            bn::string_view _title;
            scene_type _type;
            bn::optional<bn::regular_bg_ptr> _background;
            bn::optional<pop_up> _confirmation_pop_up;
            bn::sprite_text_generator _text_generator_8x16;
            bn::sprite_text_generator _text_generator_8x8;
            bn::vector<bn::sprite_ptr, dump_scene_max_text_sprite_count> _text_sprites;
            bn::vector<bn::sprite_ptr, 4> _sprites;

            cart_infos _current_cart_infos;
            request_status _request_status;
            bn::optional<pop_up> _popup;

            void set_content_priority(int priority);

        public:
            dump_rom_info_scene();
            virtual ~dump_rom_info_scene() = default;
            virtual void enter();
            virtual void exit();
            virtual void update();
            virtual void render();
            virtual scene_type get_scene_type();
            virtual void set_title(const bn::string_view &title);
            void render_cart_infos();
        };
    }
}

#endif // DUMP_ROM_INFO_SCENE_H
