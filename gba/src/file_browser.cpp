#include "bn_string.h"

#include "bn_keypad.h"
#include "bn_algorithm.h"
#include "bn_sprite_items_file.h"
#include "bn_sprite_items_save.h"
#include "bn_sprite_items_folder.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_gbacart.h"

#include "api/cart_api.h"
#include "file_browser.h"
#include "flash_context.h"
#include "api/rom_info_api.h"
#include "api/save_info_api.h"
#include "api/filesystem_api.h"
#include "scene_state_machine.h"
#include "utilities/text_helpers.h"
#include "common_variable_8x16_sprite_font.h"

namespace openflash
{
    namespace gba
    {
        file_browser::file_browser(bn::optional<file_type> file_filter) :
            _text_generator(common::variable_8x16_sprite_font),
            _text_sprites(),
            _cursor_sprite(bn::sprite_items::cursor.create_sprite(screen_left + 12, file_y)),
            _icons(),
            _empty_folder_popup(),
            _browser_state(),
            _history(),
            _current_path("/"),
            _restore_history(true),
            _file_filter(file_filter),
            _pending_cart_infos_request(false),
            _requested_scene_type(),
            _loading_files(true)
        {
            /*
             * Pack several characters in each generated text sprite.
             *
             * Long SD-card filenames otherwise create too many sprite tile
             * items, especially when the page changes.
             */
            _text_generator.set_one_sprite_per_character(false);
            _text_generator.set_bg_priority(1);

            _cursor_sprite.set_bg_priority(1);
            _cursor_sprite.set_visible(false);
        }

        const file_entry &file_browser::current_file(int index) const
        {
            const auto &files =
                api::filesystem_api::instance().get_files_response();

            return files[index];
        }

        void file_browser::set_current_path(const bn::string_view &path)
        {
            _current_path.clear();

            for (char character : path)
            {
                if (_current_path.full())
                    break;

                _current_path.push_back(character);
            }

            if (_current_path.empty())
                _current_path = "/";
        }

        void file_browser::request_files(const bn::string_view &path)
        {
            set_current_path(path);

            // A folder-specific popup must never survive navigation.
            _empty_folder_popup.reset();

            /*
             * Mark the browser as loading immediately, before sending the
             * request.
             *
             * This matters because a small directory can answer before the next
             * frame. The scene must not infer "loading" only from whether the
             * response is still pending on the following frame.
             */
            _loading_files = true;

            /*
             * Release the previous directory rows immediately. The loading
             * popup is rendered by file_browser_scene in the same frame in
             * which this request starts.
             */
            _text_sprites.clear();
            _icons.clear();

            _browser_state.need_update = true;
            _cursor_sprite.set_visible(false);

            api::filesystem_api::instance().request_files(
                _file_filter,
                path);
        }

        bool file_browser::files_load_pending()
        {
            if (!_loading_files)
                return false;

            auto &filesystem =
                api::filesystem_api::instance();

            filesystem.update();

            if (!filesystem.response_available())
                return true;

            const auto &files =
                filesystem.get_files_response();

            _loading_files = false;
            _cursor_sprite.set_visible(!files.empty());
            _browser_state.need_update = true;

            return false;
        }

        bool file_browser::render_file_list()
        {
            if (!_browser_state.need_update)
                return false;

            auto &filesystem =
                api::filesystem_api::instance();

            if (!filesystem.response_available())
                return false;

            _browser_state.need_update = false;

            /*
             * Page changes are rendered on the following game frame.
             *
             * The old page therefore remains on-screen until this replacement
             * page is ready. There is no deliberately blank frame anymore.
             */
            _text_sprites.clear();
            _icons.clear();

            const auto &files =
                filesystem.get_files_response();

            if (files.empty())
            {
                _browser_state.current_file_index = 0;
                _cursor_sprite.set_visible(false);

                /*
                 * Show the empty-folder popup once for this directory.
                 *
                 * When B closes it, update() immediately navigates to the
                 * parent before render_file_list() can create it again.
                 */
                if (!_empty_folder_popup)
                {
                    _empty_folder_popup.emplace(
                        "Empty folder!",
                        true,
                        false,
                        &_cursor_sprite);

                    _empty_folder_popup->render();
                }

                return false;
            }

            _cursor_sprite.set_visible(true);

            if (_browser_state.current_file_index < 0)
                _browser_state.current_file_index = 0;

            if (_browser_state.current_file_index >= files.size())
                _browser_state.current_file_index = files.size() - 1;

            const auto &selected_file =
                current_file(
                    _browser_state.current_file_index);

            const auto selected_name =
                selected_file.name();

            const int lower_boundary =
                (_browser_state.current_file_index /
                 max_file_count_pagination) *
                max_file_count_pagination;

            const int upper_boundary =
                bn::min(
                    lower_boundary + max_file_count_pagination,
                    files.size());

            const auto parent_path =
                selected_file.path.substr(
                    0,
                    selected_file.path.size() -
                        selected_name.size());

            const auto displayed_parent_path =
                text_helpers::truncate_text(
                    parent_path,
                    max_file_character_length);

            text_helpers::draw_left(
                _text_generator,
                displayed_parent_path,
                screen_left + 20,
                screen_top + 30,
                _text_sprites);

            /*
             * This text does not change when moving the cursor inside the same
             * page, so DOWN/UP only moves the cursor sprite.
             */
            bn::string<32> range_text =
                bn::to_string<16>(lower_boundary + 1) +
                "-" +
                bn::to_string<16>(upper_boundary) +
                "/" +
                bn::to_string<16>(files.size());

            text_helpers::draw_right(
                _text_generator,
                range_text,
                bn::display::width() / 3 + 20,
                screen_top + 30,
                _text_sprites);

            for (int index = lower_boundary;
                 index < upper_boundary;
                 ++index)
            {
                const auto &file =
                    current_file(index);

                const auto file_name =
                    file.name();

                bn::string<max_file_character_length + 3> text;

                for (int i = 0;
                     i < file_name.size() &&
                     text.size() < max_file_character_length;
                     i++)
                {
                    const uint8_t character =
                        static_cast<uint8_t>(
                            file_name[i]);

                    if (character >= 32 &&
                        character <= 126)
                    {
                        text.push_back(
                            static_cast<char>(
                                character));
                    }
                    else if ((character & 0xC0) != 0x80)
                    {
                        /*
                         * The current font does not contain all UTF-8 glyphs
                         * present in the real SD card. Keep navigation usable
                         * instead of stopping at the first accented character.
                         */
                        text.push_back('?');
                    }
                }

                if (file.is_folder() &&
                    file_name != "/" &&
                    file_name != ".." &&
                    file_name != ".")
                {
                    text += "/";
                }

                const int icon_x =
                    file_x - 12;

                const int icon_y =
                    file_y +
                    (index - lower_boundary) *
                        text_spacing_y;

                if (file.is_folder())
                {
                    _icons.push_back(
                        bn::sprite_items::folder.create_sprite(
                            icon_x,
                            icon_y));
                }
                else if (file.is_file())
                {
                    _icons.push_back(
                        bn::sprite_items::file.create_sprite(
                            icon_x,
                            icon_y));
                }
                else if (file.is_gba_file())
                {
                    _icons.push_back(
                        bn::sprite_items::gbacart.create_sprite(
                            icon_x,
                            icon_y));
                }
                else
                {
                    _icons.push_back(
                        bn::sprite_items::save.create_sprite(
                            icon_x,
                            icon_y));
                }

                _icons.back().set_bg_priority(1);

                text_helpers::draw_left(
                    _text_generator,
                    text,
                    file_x,
                    icon_y,
                    _text_sprites);

                bn::string<32> size_text;

                if (file.is_folder())
                {
                    size_text = "<DIR>";
                }
                else if (file.size < 1024 * 1024)
                {
                    size_text =
                        bn::to_string<32>(
                            file.size / 1024) +
                        "KiB";
                }
                else
                {
                    size_text =
                        bn::to_string<32>(
                            file.size / (1024 * 1024)) +
                        "MiB";
                }

                text_helpers::draw_right(
                    _text_generator,
                    size_text,
                    bn::display::width() / 3 + 25,
                    icon_y,
                    _text_sprites);
            }

            update_cursor_position();

            return false;
        }

        void file_browser::request_parent_folder()
        {
            if (_history.empty())
            {
                _restore_history = false;

                scene_state_machine::instance().request_scene_state(
                    scene_type::MAIN_MENU);

                return;
            }

            _browser_state.current_file_index =
                _history.back();

            _history.pop_back();

            if (_current_path == "/")
                return;

            int last_slash = -1;

            for (int index = 0;
                 index < _current_path.size();
                 ++index)
            {
                if (_current_path[index] == '/')
                    last_slash = index;
            }

            bn::string<max_file_path_character> parent_path;

            if (last_slash <= 0)
            {
                parent_path = "/";
            }
            else
            {
                for (int index = 0;
                     index < last_slash;
                     ++index)
                {
                    parent_path.push_back(
                        _current_path[index]);
                }
            }

            request_files(parent_path);
        }

        void file_browser::update()
        {
            /*
             * Empty-folder popup behavior:
             *
             * B closes the popup and immediately returns to the parent.
             * Because request_parent_folder() starts a new directory request,
             * the empty popup cannot be recreated in a loop.
             */
            if (_empty_folder_popup)
            {
                _empty_folder_popup->update();

                if (!_empty_folder_popup->is_open())
                {
                    _empty_folder_popup.reset();
                    request_parent_folder();
                }

                return;
            }

            if (_pending_cart_infos_request)
            {
                auto &cart_api =
                    api::cart_api::instance();

                cart_api.update();

                if (!cart_api.response_available())
                    return;

                const auto &cart_infos =
                    cart_api.get_cart_infos_response();

                flash_context::instance().set_current_cart_infos(
                    cart_infos);

                _pending_cart_infos_request = false;

                scene_state_machine::instance().request_scene_state(
                    _requested_scene_type);

                return;
            }

            const auto &files =
                api::filesystem_api::instance().get_files_response();

            /*
             * render_file_list() normally creates the popup before update().
             * Keep this fallback in case the scene update order changes later.
             */
            if (files.empty())
            {
                if (!_empty_folder_popup)
                {
                    _empty_folder_popup.emplace(
                        "Empty folder!",
                        true,
                        false,
                        &_cursor_sprite);

                    _empty_folder_popup->render();
                }

                return;
            }

            if (bn::keypad::down_pressed())
            {
                if (_browser_state.current_file_index <
                    files.size() - 1)
                {
                    const int old_page =
                        _browser_state.current_file_index /
                        max_file_count_pagination;

                    _browser_state.current_file_index++;

                    const int new_page =
                        _browser_state.current_file_index /
                        max_file_count_pagination;

                    if (new_page != old_page)
                    {
                        /*
                         * Do not redraw here.
                         *
                         * The scene will render the new page at the beginning of
                         * the next frame. The old page stays visible until then,
                         * avoiding the blank-frame flicker.
                         */
                        _browser_state.need_update = true;
                        _cursor_sprite.set_visible(false);
                    }
                    else
                    {
                        update_cursor_position();
                    }
                }

                return;
            }

            if (bn::keypad::up_pressed())
            {
                if (_browser_state.current_file_index > 0)
                {
                    const int old_page =
                        _browser_state.current_file_index /
                        max_file_count_pagination;

                    _browser_state.current_file_index--;

                    const int new_page =
                        _browser_state.current_file_index /
                        max_file_count_pagination;

                    if (new_page != old_page)
                    {
                        _browser_state.need_update = true;
                        _cursor_sprite.set_visible(false);
                    }
                    else
                    {
                        update_cursor_position();
                    }
                }

                return;
            }

            if (bn::keypad::right_pressed())
            {
                const int old_page =
                    _browser_state.current_file_index /
                    max_file_count_pagination;

                _browser_state.current_file_index +=
                    max_file_count_pagination;

                if (_browser_state.current_file_index >=
                    files.size())
                {
                    _browser_state.current_file_index =
                        files.size() - 1;
                }

                const int new_page =
                    _browser_state.current_file_index /
                    max_file_count_pagination;

                if (new_page != old_page)
                {
                    _browser_state.need_update = true;
                    _cursor_sprite.set_visible(false);
                }
                else
                {
                    update_cursor_position();
                }

                return;
            }

            if (bn::keypad::left_pressed())
            {
                const int old_page =
                    _browser_state.current_file_index /
                    max_file_count_pagination;

                _browser_state.current_file_index -=
                    max_file_count_pagination;

                if (_browser_state.current_file_index < 0)
                    _browser_state.current_file_index = 0;

                const int new_page =
                    _browser_state.current_file_index /
                    max_file_count_pagination;

                if (new_page != old_page)
                {
                    _browser_state.need_update = true;
                    _cursor_sprite.set_visible(false);
                }
                else
                {
                    update_cursor_position();
                }

                return;
            }

            if (bn::keypad::a_pressed())
            {
                const auto &file =
                    current_file(
                        _browser_state.current_file_index);

                if (file.is_folder())
                {
                    _history.emplace_back(
                        _browser_state.current_file_index);

                    _browser_state.current_folder_id =
                        file.id;

                    _browser_state.current_file_index = 0;

                    request_files(
                        file.path);

                    return;
                }

                if (file.is_gba_file())
                {
                    auto rom_infos =
                        api::rom_info_api::instance().get_current_rom_infos(
                            file);

                    if (!rom_infos.has_value())
                        return;

                    flash_context::instance().set_current_rom_infos(
                        rom_infos.value());

                    _requested_scene_type =
                        scene_type::FLASH_SCREEN;

                    _pending_cart_infos_request = true;

                    api::cart_api::instance().request_cart_infos();

                    return;
                }

                if (file.is_save_file())
                {
                    auto save_infos =
                        api::save_info_api::instance().get_current_save_infos(
                            file);

                    if (!save_infos.has_value())
                        return;

                    flash_context::instance().set_current_save_infos(
                        save_infos.value());

                    _requested_scene_type =
                        scene_type::SAVE_PROCESS_SCREEN;

                    _pending_cart_infos_request = true;

                    api::cart_api::instance().request_cart_infos();

                    return;
                }

                return;
            }

            if (bn::keypad::b_pressed())
            {
                request_parent_folder();
                return;
            }
        }

        void file_browser::update_cursor_position()
        {
            const auto &files =
                api::filesystem_api::instance().get_files_response();

            if (files.empty())
            {
                _cursor_sprite.set_visible(false);
                return;
            }

            const int lower_boundary =
                (_browser_state.current_file_index /
                 max_file_count_pagination) *
                max_file_count_pagination;

            const int visible_index =
                _browser_state.current_file_index -
                lower_boundary;

            _cursor_sprite.set_visible(true);

            _cursor_sprite.set_y(
                file_y +
                visible_index *
                    text_spacing_y);
        }

        bool file_browser::restore_browser_state()
        {
            return _restore_history;
        }

        bool openflash::gba::file_browser::is_files_loading()
        {
            return _loading_files;
        }
    }
}
