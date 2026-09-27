#include "protocol.h"
#include "utilities.h"
#include "request_handler.h"
#include "interfaces/rom_infos.h"

namespace openflash
{
    namespace esp32
    {
        request_handler::request_handler(i_filesystem &filesystem) :
            _filesystem(filesystem)
        {
        }

        protocol request_handler::handle(const protocol &request)
        {
            protocol response;

            response.protocol_version = current_protocol_version;
            response.sequence_number = request.sequence_number;
            response.cmd = request.cmd;
            std::vector<uint8_t> &payload = response.payload;

            switch (request.cmd)
            {
                case command::PING:
                    if (!request.payload.empty())
                        append_u8(payload, static_cast<uint8_t>(status::INVALID_PAYLOAD));
                    else
                        handle_ping(payload);
                    break;

                case command::GET_CART_INFO:
                    if (!request.payload.empty())
                        append_u8(payload, static_cast<uint8_t>(status::INVALID_PAYLOAD));
                    else
                        handle_get_cart_infos(payload);
                    break;

                case command::LIST_FILES:
                    handle_list_files(request.payload, payload);
                    break;

                default:
                    append_u8(payload, static_cast<uint8_t>(status::INVALID_COMMAND));
                    break;
            }

            return response;
        }

        void request_handler::handle_ping(std::vector<uint8_t> &payload)
        {
            append_u8(payload, static_cast<uint8_t>(status::OK));
        }

        void request_handler::handle_get_cart_infos(std::vector<uint8_t> &payload)
        {
            append_u8(payload, static_cast<uint8_t>(status::OK));

            // Fake cart for now
            rom_infos infos;
            infos.complement_checksum = 0;
            infos.file_path = "/GAMES/PokemonEmerald.gba";
            infos.game_code = "BPEE";
            infos.header_valid = true;
            infos.maker_code = "0";
            infos.name = "POKEMON EMER";
            infos.savetype = save_type::FLASH_128K;

            // file path
            append_string(payload, infos.file_path);
            // name
            append_string(payload, infos.name);
            // game code
            append_string(payload, infos.game_code);
            // maker code
            append_string(payload, infos.maker_code);
            // header valid
            append_u8(payload, infos.header_valid);
            // save type
            append_u8(payload, static_cast<uint8_t>(infos.savetype));
        }

        void request_handler::handle_list_files(const std::vector<uint8_t> &request, std::vector<uint8_t> &payload)
        {
            size_t offset = 0;
            std::string request_path;
            if (!read_string(request, offset, request_path))
            {
                append_u8(payload, static_cast<uint8_t>(status::INVALID_PAYLOAD));
                return;
            }

            if (offset != request.size())
            {
                append_u8(payload, static_cast<uint8_t>(status::INVALID_PAYLOAD));

                return;
            }

            if (!_filesystem.ready())
            {
                append_u8(payload, static_cast<uint8_t>(status::NOT_READY));

                return;
            }

            std::vector<file_entry> files;
            if (!_filesystem.list_directory(request_path, files))
            {
                append_u8(payload, static_cast<uint8_t>(status::NOT_FOUND));
                return;
            }

            // if success, setting payload status as OK
            append_u8(payload, static_cast<uint8_t>(status::OK));

            // files list size
            append_u16(payload, files.size());

            // payload
            for (const auto &file : files)
            {
                // type
                append_u8(payload, static_cast<uint8_t>(file.type));

                // size
                append_u32(payload, file.size);

                // path
                append_string(payload, file.path);
            }
        }
    }
}
