#ifndef GBA_UART_H
#define GBA_UART_H

#include "bn_string.h"

#include <cstdint>

#include "common.h"
#include "protocol.h"

namespace openflash
{
    namespace gba
    {
#ifdef USELUASERVER

        static constexpr uint16_t emulator_uart_buffer_size = 8192;

        struct emulator_uart_mailbox
        {
            volatile uint16_t tx_read;
            volatile uint16_t tx_write;

            volatile uint16_t rx_read;
            volatile uint16_t rx_write;

            volatile uint8_t tx_overflow;
            volatile uint8_t rx_overflow;

            volatile uint8_t reserved[2];

            volatile uint8_t tx_data[emulator_uart_buffer_size];
            volatile uint8_t rx_data[emulator_uart_buffer_size];
        };

        extern emulator_uart_mailbox mailbox;

#endif

        void uart_init();

        void uart_send(uint8_t byte);

        bool uart_receive(uint8_t &byte);

        template <int MaxSize> void append_char(bn::string<MaxSize> &output, char value)
        {
            if (output.size() < MaxSize)
                output.push_back(value);
        }

        template <int MaxSize> void append_text(bn::string<MaxSize> &output, const char *text)
        {
            while (*text != '\0' && output.size() < MaxSize)
            {
                output.push_back(*text);
                text++;
            }
        }

        template <int MaxSize> void append_decimal(bn::string<MaxSize> &output, uint32_t value)
        {
            char buffer[10];
            int index = 0;

            if (value == 0)
            {
                append_char(output, '0');
                return;
            }

            while (value > 0)
            {
                buffer[index++] = static_cast<char>('0' + value % 10);

                value /= 10;
            }

            while (index > 0)
            {
                index--;

                append_char(output, buffer[index]);
            }
        }

        template <int MaxSize> void append_hex_byte(bn::string<MaxSize> &output, uint8_t value)
        {
            static constexpr char hex[] = "0123456789ABCDEF";

            append_char(output, hex[(value >> 4) & 0x0F]);

            append_char(output, hex[value & 0x0F]);
        }

        inline const char *command_name(openflash::command cmd)
        {
            switch (cmd)
            {
                case openflash::command::PING:
                    return "PING";

                case openflash::command::GET_CART_INFO:
                    return "GET_CART_INFO";

                case openflash::command::LIST_FILES:
                    return "LIST_FILES";

                case openflash::command::GET_ROM_INFO:
                    return "GET_ROM_INFO";

                case openflash::command::GET_SAVE_INFO:
                    return "GET_SAVE_INFO";

                case openflash::command::START_PROCESS:
                    return "START_PROCESS";

                case openflash::command::GET_PROCESS_INFO:
                    return "GET_PROCESS_INFO";

                case openflash::command::RESET_PROCESS:
                    return "RESET_PROCESS";

                case openflash::command::DEBUG:
                    return "DEBUG";

                default:
                    return "UNKNOWN";
            }
        }

        template <int MaxSize> void received_packet_to_string(const protocol &packet, bn::string<MaxSize> &output)
        {
            output.clear();

            append_text(output, "Protocol Version : ");

            append_decimal(output, packet.protocol_version);

            append_char(output, '\n');

            append_text(output, "Sequence Number  : ");

            append_decimal(output, packet.sequence_number);

            append_char(output, '\n');

            append_text(output, "Command          : ");

            append_text(output, command_name(packet.cmd));

            append_text(output, " (0x");

            append_hex_byte(output, static_cast<uint8_t>(packet.cmd));

            append_char(output, ')');

            append_char(output, '\n');

            append_text(output, "Payload Size     : ");

            append_decimal(output, static_cast<uint32_t>(packet.payload.size()));

            append_char(output, '\n');

            append_text(output, "Payload:\n");

            for (int i = 0; i < packet.payload.size(); i++)
            {
                /*
                 * 2 hex chars + optional space.
                 */
                int required = i + 1 < packet.payload.size() ? 3 : 2;

                if (output.size() + required > MaxSize)
                {
                    append_text(output, "...");

                    break;
                }

                append_hex_byte(output, packet.payload[i]);

                if (i + 1 < packet.payload.size())
                {
                    append_char(output, ' ');
                }
            }
        }
    }
}

#endif