#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <vector>

#include "../interfaces/file_entry.h"

namespace openflash
{
    namespace esp32
    {
        class i_filesystem
        {
        public:
            virtual ~i_filesystem() = default;
            virtual bool ready() const = 0;
            virtual bool list_directory(const std::string &path, std::vector<file_entry> &entries) = 0;
        };
    }
}

#endif