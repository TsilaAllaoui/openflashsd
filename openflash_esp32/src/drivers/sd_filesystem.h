#ifndef SD_FILESYSTEM_H
#define SD_FILESYSTEM_H

#include "i_filesystem.h"
#include <FS.h>

namespace openflash
{
    namespace esp32
    {
        class sd_filesystem : public i_filesystem
        {
        private:
            bool _ready = false;
            file_type get_file_type(File file);

        public:
            bool begin();
            bool ready() const;
            bool list_directory(const std::string &path, std::vector<file_entry> &entries);
        };
    }
}

#endif