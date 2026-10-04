#ifndef FILE_ENTRY_H
#define FILE_ENTRY_H

#include <stdint.h>

#include "bn_display.h"
#include "bn_optional.h"
#include "bn_string.h"
#include "bn_string_view.h"
#include "bn_vector.h"

#include "common.h"
#include "constants.h"
#include "communication/utilities.h"

namespace openflash
{
    namespace gba
    {
        struct rom_infos;

        class file_entry
        {
        public:
            file_entry(bn::string_view path_,
                       file_type type_,
                       int16_t id_,
                       int16_t parentId_ = -1,
                       uint8_t depth_ = 0,
                       uint32_t size_ = 0);

            ~file_entry() = default;

            bn::string<stored_path_size> path;

            uint32_t size;
            int16_t id;
            int16_t parentId;
            file_type type;
            uint8_t depth;

            bn::string<stored_path_size> name() const;
            bn::string<stored_path_size> nth_parent(int n = 1) const;

            bool is_folder() const;
            bool is_file() const;
            bool is_gba_file() const;
            bool is_save_file() const;

            rom_infos get_gba_file_info(uint8_t *rom_bytes);
        };
    }
}

#endif