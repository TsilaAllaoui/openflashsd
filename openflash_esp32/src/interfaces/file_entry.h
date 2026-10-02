#ifndef FILE_ENTRY_H
#define FILE_ENTRY_H

#include <string>
#include <stdint.h>

#include "common.h"

namespace openflash
{
    namespace esp32
    {
        struct file_entry
        {
            std::string path;
            uint32_t size;
            int16_t id;
            int16_t parentId;
            file_type type;
            uint8_t depth;

            file_entry()
            {
                size = 0;
                type = file_type::FOLDER;
                path = "";
            }

            file_entry(uint32_t size_, file_type type_, std::string path_)
            {
                size = size_;
                type = type_;
                path = path_;
            }
        };
    }
}

#endif // FILE_ENTRY_H
