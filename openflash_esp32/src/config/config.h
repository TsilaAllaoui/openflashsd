#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

namespace openflash
{
    namespace esp32
    {
        struct Config
        {
            uint32_t max_payload_size = 4096;
            uint32_t max_sd_frequency = 8000000;
        };

        extern Config config;
    }
}

#endif // CONFIG_H