#include "save_infos.h"
#include "api/filesystem_api.h"
#include "communication/utilities.h"
#include "communication/packet_decoder.h"

#include "bn_log.h"

#include <type_traits>

#ifdef USEMOCK
#include "mock/mocks.h"
#endif

namespace openflash
{
    namespace gba
    {
        namespace api
        {
            filesystem_api::filesystem_api() :
                _files(),
                _file_filter(),
                _loading(false),
                _response_ready(false)
#ifdef USEMOCK
                ,
                _mock_frames(0)
#endif
            {
            }

            filesystem_api &filesystem_api::instance()
            {
                static filesystem_api api;
                return api;
            }

            void filesystem_api::request_files(bn::optional<file_type> file_filter, const bn::string_view &payload)
            {
                _file_filter = file_filter;
                _files.clear();
                _loading = true;
                _response_ready = false;
#ifdef USEMOCK
                _mock_frames = 30;
#else
                //requesting files from esp32
                gba::send_packet(std::remove_const_t<uint8_t *>(payload.data()),
                                 payload.size(),
                                 openflash::command::LIST_FILES);
#endif
            }

            void filesystem_api::update()
            {
                if (!_loading)
                    return;
#ifdef USEMOCK
                if (_mock_frames > 0)
                {
                    _mock_frames--;
                    return;
                }

                const auto &files = mock::mock_file_entries();

                for (const auto &file : files)
                {
                    if (_file_filter.has_value() && file.type != *_file_filter && file.type != file_type::FOLDER)
                        continue;

                    _files.emplace_back(file);
                }

                _loading = false;
                _response_ready = true;
#else
                // if esp32 responded, drains all bytes received in one frame
                openflash::gba::protocol packet;
                while (openflash::gba::receive_packet(packet))
                {
                    auto file_count = packet.payload[3] | (packet.payload[4] << 8);

                    size_t offset = 5;
                    while (_files.size() < file_count)
                    {
                        auto type = static_cast<file_type>(packet.payload[offset++]);

                        uint32_t size = 0;
                        gba::read_u32(packet.payload.data(), offset, size);

                        bn::string<gba::max_file_name_character_count> path;
                        gba::read_string(packet.payload.data(), offset, path);

                        const int16_t id = static_cast<int16_t>(_files.size());

                        _files.emplace_back(path, type, id, -1, 0, size);
                    }

                    _loading = false;
                    _response_ready = true;
                }
#endif
            }

            bool filesystem_api::response_available() const
            {
                return _response_ready;
            }

            const bn::vector<file_entry, max_file_count> &filesystem_api::get_files_response() const
            {
                return _files;
            }
        }
    }
}