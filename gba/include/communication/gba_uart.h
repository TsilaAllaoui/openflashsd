#ifndef GBA_UART_H
#define GBA_UART_H

#include <cstdint>

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
    }
}

#endif