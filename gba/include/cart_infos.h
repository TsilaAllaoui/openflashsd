#ifndef CART_INFOS_H
#define CART_INFOS_H

#include "bn_string.h"
#include "rom_infos.h"
#include "constants.h"

namespace openflash
{
    namespace gba
    {
        struct cart_infos
        {
            bn::string<max_cart_character_name> name;
            rom_infos cart_rom_infos;
        };
    }
}

#endif // CART_INFOS_H