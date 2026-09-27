#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include "sd_filesystem.h"
#include "../interfaces/file_entry.h"

namespace openflash
{
    namespace esp32
    {
        constexpr int SD_SCK = 0;
        constexpr int SD_MISO = 1;
        constexpr int SD_MOSI = 3;
        constexpr int SD_CS = 10;

        file_type sd_filesystem::get_file_type(File file)
        {
            String fileName = file.name();

            if (fileName.endsWith(".gba"))
                return file_type::GBA_FILE;
            else if (fileName.endsWith(".sav"))
                return file_type::SAVE_FILE;
            else if (file.isDirectory())
                return file_type::FOLDER;
            return file_type::NORMAL_FILE;
        }

        bool sd_filesystem::begin()
        {
            SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

            _ready = SD.begin(SD_CS, SPI, 10000000);

            if (!_ready)
                return false;

            if (SD.cardType() == CARD_NONE)
            {
                _ready = false;
                return false;
            }

            return true;
        }

        bool sd_filesystem::ready() const
        {
            return _ready;
        }

        bool sd_filesystem::list_directory(const std::string &path, std::vector<file_entry> &entries)
        {
            entries.clear();

            if (!_ready)
                return false;

            File root = SD.open(path.c_str());

            if (!root)
                return false;

            if (!root.isDirectory())
            {
                root.close();
                return false;
            }

            File file = root.openNextFile();

            while (file)
            {
                file_entry entry;
                entry.path = file.path();
                entry.type = get_file_type(file);
                entry.size = file.isDirectory() ? 0 : static_cast<uint32_t>(file.size());
                entries.emplace_back(entry);
                file.close();
                file = root.openNextFile();
            }

            root.close();

            return true;
        }
    }
}