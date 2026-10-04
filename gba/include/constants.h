#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "bn_display.h"

namespace openflash
{
    namespace gba
    {
        constexpr int title_x = -4;
        constexpr int title_y = 16;

        constexpr int max_character_count = 64;
        constexpr int max_navigation_depth = 16;
        constexpr int max_file_character_length = 22;

        constexpr int screen_left = -(bn::display::width() / 2);
        constexpr int screen_top = -(bn::display::height() / 2);
        constexpr int file_x = screen_left + 40;
        constexpr int text_spacing_y = 14;
        constexpr int file_y = screen_top + 44;
        constexpr int max_file_count_pagination = 8;
        constexpr int max_file_count = 100;

        constexpr int max_file_path_character = 128;

        constexpr int max_file_name_character_count = 32;

        constexpr int max_cart_character_name = 100;

        constexpr int stored_path_size = 32;

        constexpr int dump_scene_max_text_sprite_count = 100;
        constexpr int dump_scene_text_y_spacing = 14;
        constexpr int dump_scene_text_y_top = -bn::display::height() / 2 + 30;
        constexpr int dump_x_alignment = -bn::display::width() / 2 + 20;

        constexpr int flash_scene_max_text_sprite_count = 100;
        constexpr int flash_scene_text_y_spacing = 14;
        constexpr int flash_scene_text_y_top = -bn::display::height() / 2 + 30;
        constexpr int flash_x_alignment = -bn::display::width() / 2 + 20;

        constexpr int process_progress_scene_max_text_sprite_count = 100;
        constexpr int process_progress_scene_text_y_spacing = 14;
        constexpr int process_progress_scene_text_y_top = -bn::display::height() / 2 + 30;
        constexpr int process_progress_x_alignment = -bn::display::width() / 2 + 20;

        constexpr int process_save_scene_max_text_sprite_count = 100;
        constexpr int process_save_scene_text_y_spacing = 14;
        constexpr int process_save_scene_text_y_top = -bn::display::height() / 2 + 30;
        constexpr int process_save_x_alignment = -bn::display::width() / 2 + 20;

        constexpr int save_process_selection_scene_max_text_sprite_count = 100;
        constexpr int save_process_selection_scene_text_y_spacing = 14;
        constexpr int save_process_selection_scene_text_y_top = -bn::display::height() / 2 + 30;
        constexpr int save_process_selection_x_alignment = -bn::display::width() / 2 + 20;

        constexpr int selector_width = 48;
        constexpr int title_offset_y = 12;

        constexpr int map_width = 32;
        constexpr int map_height = 32;
        constexpr int map_cell_count = map_width * map_height;
    }
}

#endif // CONSTANTS_H