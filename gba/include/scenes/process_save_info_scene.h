#ifndef PROCESS_SAVE_INFO_SCENE_H
#define PROCESS_SAVE_INFO_SCENE_H

#include "bn_display.h"
#include "bn_sprite_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_text_generator.h"

#include "constants.h"
#include "cart_infos.h"
#include "scenes/i_scene.h"
#include "utilities/pop_up.h"
#include "scenes/scene_type.h"

namespace openflash
{
    namespace gba
    {
        class process_save_info_scene : public i_scene
        {
        private:
            bn::string_view _title;
            scene_type _type;
            bn::optional<bn::regular_bg_ptr> _background;
            bn::optional<pop_up> _pop_up;
            bn::sprite_text_generator _text_generator_8x16;
            bn::sprite_text_generator _text_generator_8x8;
            bn::vector<bn::sprite_ptr, process_save_scene_max_text_sprite_count> _text_sprites;

            int gbacart_index = 0;
            int arrow_index = 1;
            int save_1_index = 2;
            int save_2_index = 3;
            bn::vector<bn::sprite_ptr, 4> _sprites;

            cart_infos _current_cart_infos;
            request_status _request_status;

            void set_content_priority(int priority);

        public:
            process_save_info_scene();
            virtual ~process_save_info_scene() = default;
            virtual void enter();
            virtual void exit();
            virtual void update();
            virtual void render();
            virtual scene_type get_scene_type();
            virtual void set_title(const bn::string_view &title);
            void render_infos();
        };
    } // namespace openflash
}

#endif // PROCESS_SAVE_INFO_SCENE_H
