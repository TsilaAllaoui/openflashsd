#ifndef SAVE_INFO_H
#define SAVE_INFO_H

#include <string>

namespace openflash
{
    namespace esp32
    {
        enum class save_type : uint8_t
        {
            NONE = 0,
            EEPROM_512B, // 4 Kbit
            EEPROM_8K,   // 64 Kbit
            SRAM_32K,    // 256 Kbit
            FLASH_64K,   // 512 Kbit
            FLASH_128K,  // 1 Mbit
            FRAM_32K,
            FRAM_64K,
            FRAM_128K,
            UNKNOWN
        };
    }
}

#endif // SAVE_INFO_H