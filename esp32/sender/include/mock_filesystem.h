#ifndef MOCK_FILESYSTEM_H
#define MOCK_FILESYSTEM_H

#include "drivers/i_filesystem.h"

namespace openflash
{
    namespace esp32
    {
        class mock_filesystem : public i_filesystem
        {
        private:
            bool _ready;

        public:
            mock_filesystem();
            virtual bool ready() const;
            virtual bool list_directory(const std::string &path, std::vector<file_entry> &entries);
        };
    }
}

#endif // MOCK_FILESYSTEM_H