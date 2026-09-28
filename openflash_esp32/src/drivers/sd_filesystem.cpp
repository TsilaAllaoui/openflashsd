#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include <filesystem>

#include "../protocol.h"
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
        constexpr uint32_t SD_BOOTSTRAP_FREQUENCY = 8000000;

        file_type sd_filesystem::get_file_type(const std::string &path, bool is_directory)
        {
            if (is_directory)
                return file_type::FOLDER;

            if (path.size() >= 4 && path.compare(path.size() - 4, 4, ".gba") == 0)
                return file_type::GBA_FILE;

            if (path.size() >= 4 && path.compare(path.size() - 4, 4, ".sav") == 0)
                return file_type::SAVE_FILE;

            return file_type::NORMAL_FILE;
        }

        bool load_config(const char *path)
        {
            File file = SD.open(path, FILE_READ);

            if (!file)
                return false;

            while (file.available())
            {
                String line = file.readStringUntil('\n');
                line.trim();

                if (line.length() == 0 || line.startsWith("#"))
                    continue;

                int separator = line.indexOf('=');

                if (separator < 0)
                    continue;

                String key = line.substring(0, separator);
                String value = line.substring(separator + 1);

                key.trim();
                value.trim();

                if (key == "max_payload_size")
                {
                    const long parsed_value = value.toInt();

                    if (parsed_value > 0 && parsed_value <= UINT16_MAX)
                        config.max_payload_size = static_cast<uint32_t>(parsed_value);
                }
                else if (key == "max_sd_frequency")
                {
                    const long parsed_value = value.toInt();

                    if (parsed_value > 0)
                        config.max_sd_frequency = static_cast<uint32_t>(parsed_value);
                }
            }

            file.close();
            return true;
        }

        bool sd_filesystem::begin()
        {
            SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

            _ready = SD.begin(SD_CS, SPI, SD_BOOTSTRAP_FREQUENCY);

            if (!_ready)
                return false;

            if (SD.cardType() == CARD_NONE)
            {
                _ready = false;
                return false;
            }

            load_config("/openflash.cfg");

            if (config.max_sd_frequency != SD_BOOTSTRAP_FREQUENCY)
            {
                SD.end();
                _ready = SD.begin(SD_CS, SPI, config.max_sd_frequency);

                if (!_ready)
                    _ready = SD.begin(SD_CS, SPI, SD_BOOTSTRAP_FREQUENCY);
            }

            if (!_ready || SD.cardType() == CARD_NONE)
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

            while (true)
            {
                bool is_directory = false;
                String full_path = root.getNextFileName(&is_directory);

                if (full_path.length() == 0)
                    break;

                int separator = full_path.lastIndexOf('/');
                String file_name = separator >= 0 ? full_path.substring(separator + 1) : full_path;

                file_entry entry;
                entry.path = file_name.c_str();
                entry.type = get_file_type(entry.path, is_directory);
                entry.size =
                    0; // For now it's the bottleneck wo we will not use file size here, only in lazy loaiding or for only one file
                entries.emplace_back(std::move(entry));
            }

            root.close();
            return true;
        }
    }
}
