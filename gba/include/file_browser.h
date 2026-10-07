#ifndef FILE_BROWSER_H
#define FILE_BROWSER_H

#include <stdint.h>

#include "bn_string.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"

#include "constants.h"
#include "file_entry.h"
#include "scene_type.h"
#include "utilities/pop_up.h"

namespace openflash
{
    namespace gba
    {
        static_assert(max_file_count <= 255, "File indices need a larger integer type");

        struct file_browser_state
        {
            int current_folder_id = -1;
            int current_file_index = 0;
            bool need_update = true;
        };

        class file_browser
        {
        private:
            bn::sprite_text_generator _text_generator;
            bn::vector<bn::sprite_ptr, max_file_count> _text_sprites;
            bn::sprite_ptr _cursor_sprite;
            bn::vector<bn::sprite_ptr, max_file_count_pagination> _icons;
            bn::optional<pop_up> _empty_folder_popup;
            file_browser_state _browser_state;
            bn::vector<int, max_navigation_depth> _history;
            bn::string<max_file_path_character> _current_path;
            bool _restore_history;
            bn::optional<file_type> _file_filter;
            bool _pending_cart_infos_request;
            scene_type _requested_scene_type;

            const file_entry &current_file(int index) const;
            void update_cursor_position();
            void request_parent_folder();
            void set_current_path(const bn::string_view &path);

        public:
            bool _loading_files;

            file_browser(bn::optional<file_type> file_filter);
            ~file_browser() = default;

            void request_files(const bn::string_view &path);
            bool files_load_pending();
            bool render_file_list();
            void update();
            bool restore_browser_state();
            bool is_files_loading();
        };
    }
}

#endif // FILE_BROWSER_H
