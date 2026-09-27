#ifndef FILE_ENTRY_H
#define FILE_ENTRY_H

#include <stdint.h>
#include <string>

namespace openflash
{
    namespace esp32
    {
        enum class file_type : uint8_t
        {
            NORMAL_FILE,
            FOLDER,
            GBA_FILE,
            SAVE_FILE
        };

        struct file_entry
        {
            std::string path;
            uint32_t size;
            int16_t id;
            int16_t parentId;
            file_type type;
            uint8_t depth;
        };
    }
}

#endif // FILE_ENTRY_H
