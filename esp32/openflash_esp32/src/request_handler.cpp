#include "protocol.h"
#include "utilities.h"
#include "config/config.h"
#include "request_handler.h"
#include "interfaces/rom_infos.h"

namespace openflash
{
    namespace esp32
    {
        constexpr uint8_t type_size = 1;
        constexpr uint16_t file_entry_size = 4;
        constexpr uint16_t file_path_length_size = 2;
        constexpr uint16_t file_entry_minimal_size = type_size + file_entry_size + file_path_length_size;
        constexpr uint16_t file_list_header_size = 5;

        request_handler::request_handler(i_filesystem &filesystem) :
            _filesystem(filesystem)
        {
        }

        std::vector<protocol> request_handler::handle(const protocol &request)
        {
            std::vector<protocol> responses;

            switch (request.cmd)
            {
                case command::PING:
                {
                    protocol response;
                    response.protocol_version = current_protocol_version;
                    response.sequence_number = request.sequence_number;
                    response.cmd = request.cmd;

                    if (!request.payload.empty())
                        append_u8(response.payload, static_cast<uint8_t>(status::INVALID_PAYLOAD));
                    else
                        handle_ping(response.payload);

                    responses.emplace_back(response);
                    break;
                }

                case command::GET_CART_INFO:
                {
                    protocol response;
                    response.protocol_version = current_protocol_version;
                    response.sequence_number = request.sequence_number;
                    response.cmd = request.cmd;

                    if (!request.payload.empty())
                        append_u8(response.payload, static_cast<uint8_t>(status::INVALID_PAYLOAD));
                    else
                        handle_get_cart_infos(response.payload);

                    responses.emplace_back(response);
                    break;
                }

                case command::LIST_FILES:
                    handle_list_files(request, responses);
                    break;

                case command::DEBUG:
                {
                    protocol response;
                    response.protocol_version = current_protocol_version;
                    response.sequence_number = request.sequence_number;
                    response.cmd = request.cmd;
                    handle_debug(request, response.payload);
                    responses.emplace_back(response);
                    break;
                }
                default:
                {
                    protocol response;
                    response.protocol_version = current_protocol_version;
                    response.sequence_number = request.sequence_number;
                    response.cmd = request.cmd;
                    append_u8(response.payload, static_cast<uint8_t>(status::INVALID_COMMAND));
                    responses.emplace_back(response);
                    break;
                }
            }

            return responses;
        }

        void request_handler::handle_ping(std::vector<uint8_t> &payload)
        {
            append_u8(payload, static_cast<uint8_t>(status::OK));
        }

        void request_handler::handle_get_cart_infos(std::vector<uint8_t> &payload)
        {
            append_u8(payload, static_cast<uint8_t>(status::OK));

            rom_infos infos;
            infos.complement_checksum = 0;
            infos.file_path = "/GAMES/PokemonEmerald.gba";
            infos.game_code = "BPEE";
            infos.header_valid = true;
            infos.maker_code = "0";
            infos.name = "POKEMON EMER";
            infos.savetype = save_type::FLASH_128K;

            append_string(payload, infos.file_path);
            append_string(payload, infos.name);
            append_string(payload, infos.game_code);
            append_string(payload, infos.maker_code);
            append_u8(payload, infos.header_valid);
            append_u8(payload, static_cast<uint8_t>(infos.savetype));
        }

        void request_handler::handle_list_files(const protocol &request, std::vector<protocol> &responses)
        {
            size_t offset = 0;
            std::string request_path;

            if (!read_string(request.payload, offset, request_path) || offset != request.payload.size())
            {
                protocol response;
                response.protocol_version = current_protocol_version;
                response.sequence_number = request.sequence_number;
                response.cmd = request.cmd;
                append_u8(response.payload, static_cast<uint8_t>(status::INVALID_PAYLOAD));
                responses.emplace_back(response);
                return;
            }

            if (!_filesystem.ready())
            {
                protocol response;
                response.protocol_version = current_protocol_version;
                response.sequence_number = request.sequence_number;
                response.cmd = request.cmd;
                append_u8(response.payload, static_cast<uint8_t>(status::NOT_READY));
                responses.emplace_back(response);
                return;
            }

            std::vector<file_entry> files;

            if (!_filesystem.list_directory(request_path, files))
            {
                protocol response;
                response.protocol_version = current_protocol_version;
                response.sequence_number = request.sequence_number;
                response.cmd = request.cmd;
                append_u8(response.payload, static_cast<uint8_t>(status::NOT_FOUND));
                responses.emplace_back(response);
                return;
            }

            uint16_t file_count = 0;
            uint16_t estimated_size = 0;
            std::vector<uint8_t> current_response;

            for (size_t i = 0; i < files.size(); i++)
            {
                const auto &file = files[i];
                const size_t current_entry_size = file_entry_minimal_size + file.path.size();

                if (file_count > 0
                    && file_list_header_size + estimated_size + current_entry_size > config.max_payload_size)
                {
                    current_response.insert(current_response.begin(), static_cast<uint8_t>(status::NOT_FINISHED_YET));
                    current_response.insert(current_response.begin() + 1, estimated_size & 0xFF);
                    current_response.insert(current_response.begin() + 2, estimated_size >> 8);
                    current_response.insert(current_response.begin() + 3, file_count & 0xFF);
                    current_response.insert(current_response.begin() + 4, file_count >> 8);

                    protocol response;
                    response.protocol_version = current_protocol_version;
                    response.sequence_number = request.sequence_number;
                    response.cmd = request.cmd;
                    response.payload = current_response;
                    responses.emplace_back(response);

                    current_response.clear();
                    file_count = 0;
                    estimated_size = 0;
                }

                append_u8(current_response, static_cast<uint8_t>(file.type));
                append_u32(current_response, file.size);
                append_string(current_response, file.path);

                estimated_size += static_cast<uint16_t>(current_entry_size);
                file_count++;
            }

            current_response.insert(current_response.begin(), static_cast<uint8_t>(status::OK));
            current_response.insert(current_response.begin() + 1, estimated_size & 0xFF);
            current_response.insert(current_response.begin() + 2, estimated_size >> 8);
            current_response.insert(current_response.begin() + 3, file_count & 0xFF);
            current_response.insert(current_response.begin() + 4, file_count >> 8);

            protocol response;
            response.protocol_version = current_protocol_version;
            response.sequence_number = request.sequence_number;
            response.cmd = request.cmd;
            response.payload = current_response;
            responses.emplace_back(response);
        }

        void request_handler::handle_debug(const protocol &request, std::vector<uint8_t> &response)
        {
            for (auto &b : request.payload)
                response.emplace_back(b);
        }
    }
}
