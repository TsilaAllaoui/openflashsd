#include "save_infos.h"
#include "api/filesystem_api.h"
#include "communication/gba_uart.h"
#include "communication/utilities.h"
#include "communication/packet_decoder.h"

#include "bn_common.h"

#ifdef USEMOCK
#include "mock/mocks.h"
#endif

namespace openflash
{
    namespace gba
    {
        namespace api
        {
#ifndef USEMOCK
            namespace
            {
                BN_DATA_EWRAM protocol received_packet;
            }
#endif

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

            BN_DATA_EWRAM filesystem_api filesystem_api::_instance;

            filesystem_api &filesystem_api::instance()
            {
                return _instance;
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
                bn::vector<uint8_t, max_file_path_character + 2> final_payload;

                final_payload.emplace_back(
                    static_cast<uint8_t>(payload.size() & 0xFF));

                final_payload.emplace_back(
                    static_cast<uint8_t>((payload.size() >> 8) & 0xFF));

                for (char byte : payload)
                    final_payload.emplace_back(static_cast<uint8_t>(byte));

                gba::send_packet(
                    final_payload.data(),
                    static_cast<uint16_t>(final_payload.size()),
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
                    if (_file_filter.has_value() &&
                        file.type != *_file_filter &&
                        file.type != file_type::FOLDER)
                    {
                        continue;
                    }

                    _files.emplace_back(file);
                }

                _loading = false;
                _response_ready = true;
#else
                while (openflash::gba::receive_packet(received_packet))
                {
                    if (received_packet.cmd != openflash::command::LIST_FILES)
                        continue;

                    const auto &payload = received_packet.payload;

                    if (payload.empty())
                    {
                        _loading = false;
                        _response_ready = true;
                        return;
                    }

                    const auto response_status =
                        static_cast<openflash::status>(payload[0]);

                    if (response_status != openflash::status::OK &&
                        response_status != openflash::status::NOT_FINISHED_YET)
                    {
                        _loading = false;
                        _response_ready = true;
                        return;
                    }

                    if (payload.size() < 5)
                    {
                        _loading = false;
                        _response_ready = true;
                        return;
                    }

                    const uint16_t entries_size =
                        static_cast<uint16_t>(payload[1]) |
                        (static_cast<uint16_t>(payload[2]) << 8);

                    const uint16_t file_count =
                        static_cast<uint16_t>(payload[3]) |
                        (static_cast<uint16_t>(payload[4]) << 8);

                    if (static_cast<size_t>(entries_size) + 5 !=
                        static_cast<size_t>(payload.size()))
                    {
                        _loading = false;
                        _response_ready = true;
                        return;
                    }

                    size_t offset = 5;

                    for (uint16_t parsed_file_count = 0;
                         parsed_file_count < file_count;
                         parsed_file_count++)
                    {
                        if (offset >= static_cast<size_t>(payload.size()))
                        {
                            _loading = false;
                            _response_ready = true;
                            return;
                        }

                        const auto type =
                            static_cast<file_type>(
                                payload[offset++]);

                        uint32_t size = 0;

                        if (offset + 4 > static_cast<size_t>(payload.size()) ||
                            !gba::read_u32(
                                payload.data(),
                                offset,
                                size))
                        {
                            _loading = false;
                            _response_ready = true;
                            return;
                        }

                        if (offset + 2 > static_cast<size_t>(payload.size()))
                        {
                            _loading = false;
                            _response_ready = true;
                            return;
                        }

                        const uint16_t path_length =
                            static_cast<uint16_t>(payload[offset]) |
                            (static_cast<uint16_t>(payload[offset + 1]) << 8);

                        if (path_length > max_file_path_character ||
                            offset + 2 + path_length >
                                static_cast<size_t>(payload.size()))
                        {
                            _loading = false;
                            _response_ready = true;
                            return;
                        }

                        bn::string<max_file_name_character_count> path;

                        if (!gba::read_string(
                                payload.data(),
                                offset,
                                path))
                        {
                            _loading = false;
                            _response_ready = true;
                            return;
                        }

                        const bool keep_file =
                            !_file_filter.has_value() ||
                            type == *_file_filter ||
                            type == file_type::FOLDER;

                        if (keep_file && !_files.full())
                        {
                            const int16_t id =
                                static_cast<int16_t>(
                                    _files.size());

                            _files.emplace_back(
                                path,
                                type,
                                id,
                                -1,
                                0,
                                size);
                        }
                    }

                    if (offset != static_cast<size_t>(payload.size()))
                    {
                        _loading = false;
                        _response_ready = true;
                        return;
                    }

                    if (response_status == openflash::status::OK)
                    {
                        _loading = false;
                        _response_ready = true;
                        return;
                    }
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
