#ifndef ROM_INFOS_H
#define ROM_INFOS_H

#include <vector>
#include <string>
#include <cstdint>

#include "save_infos.h"

namespace openflash
{
    namespace esp32
    {
        struct rom_infos
        {
            std::string file_path;
            std::string name;
            std::string game_code;
            std::string maker_code;
            uint8_t complement_checksum;
            bool header_valid;
            save_type savetype;
        };
    }
}

#endif // ROM_INFOS_H
