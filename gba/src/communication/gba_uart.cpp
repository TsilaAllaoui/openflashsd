#include "bn_common.h"
extern "C"
{
    #include <ugba/interrupts.h>
}
#include "gba_uart.h"

namespace openflash
{
    namespace gba
    {

#ifdef USELUASERVER

        BN_DATA_EWRAM emulator_uart_mailbox mailbox = {};

        static constexpr uint16_t emulator_uart_buffer_mask = emulator_uart_buffer_size - 1;

        static_assert(
            (emulator_uart_buffer_size &
             (emulator_uart_buffer_size - 1)) == 0,
            "UART emulator buffer size must be a power of two");

        static uint16_t next_index(uint16_t index)
        {
            return static_cast<uint16_t>(
                (index + 1) &
                emulator_uart_buffer_mask);
        }

        void uart_init()
        {
            mailbox.tx_read = 0;
            mailbox.tx_write = 0;

            mailbox.rx_read = 0;
            mailbox.rx_write = 0;

            mailbox.tx_overflow = 0;
            mailbox.rx_overflow = 0;
        }

        void uart_send(uint8_t byte)
        {
            uint16_t write =
                mailbox.tx_write;

            uint16_t next =
                next_index(write);

            if (next == mailbox.tx_read)
            {
                mailbox.tx_overflow = 1;
                return;
            }

            mailbox.tx_data[write] =
                byte;

            mailbox.tx_write = next;
        }

        bool uart_receive(uint8_t &byte)
        {
            uint16_t read = mailbox.rx_read;

            if (read == mailbox.rx_write)
                return false;

            byte = mailbox.rx_data[read];

            mailbox.rx_read = next_index(read);

            return true;
        }

#else

        namespace
        {
            volatile uint16_t &REG_SIOCNT = *reinterpret_cast<volatile uint16_t *>(0x04000128);
            volatile uint16_t &REG_SIODATA8 = *reinterpret_cast<volatile uint16_t *>(0x0400012A);
            volatile uint16_t &REG_RCNT = *reinterpret_cast<volatile uint16_t *>(0x04000134);

            static constexpr uint16_t UART_BAUD_115200 = 3;

            static constexpr uint16_t UART_SEND_FULL = 1 << 4;
            static constexpr uint16_t UART_RECEIVE_EMPTY = 1 << 5;
            static constexpr uint16_t UART_ERROR = 1 << 6;
            static constexpr uint16_t UART_8_BITS = 1 << 7;
            static constexpr uint16_t UART_FIFO_ENABLE = 1 << 8;
            static constexpr uint16_t UART_SEND_ENABLE = 1 << 10;
            static constexpr uint16_t UART_RECEIVE_ENABLE = 1 << 11;
            static constexpr uint16_t UART_MODE = (1 << 12) | (1 << 13);
            static constexpr uint16_t UART_IRQ_ENABLE = 1 << 14;

            static constexpr uint16_t hardware_rx_buffer_size = 8192;
            static constexpr uint16_t hardware_rx_buffer_mask = hardware_rx_buffer_size - 1;

            static_assert(
                (hardware_rx_buffer_size &
                 (hardware_rx_buffer_size - 1)) == 0,
                "UART RX buffer size must be a power of two");

            BN_DATA_EWRAM_BSS uint8_t hardware_rx_buffer[hardware_rx_buffer_size];

            volatile uint16_t hardware_rx_read = 0;
            volatile uint16_t hardware_rx_write = 0;
            volatile bool hardware_rx_overflow = false;
            volatile bool hardware_uart_error = false;

            uint16_t next_hardware_rx_index(uint16_t index)
            {
                return static_cast<uint16_t>(
                    (index + 1) &
                    hardware_rx_buffer_mask);
            }

            void push_received_byte(uint8_t byte)
            {
                uint16_t write = hardware_rx_write;
                uint16_t next = next_hardware_rx_index(write);

                if (next == hardware_rx_read)
                {
                    hardware_rx_overflow = true;
                    return;
                }

                hardware_rx_buffer[write] = byte;
                hardware_rx_write = next;
            }

            void drain_hardware_receive_fifo()
            {
                uint16_t control = REG_SIOCNT;

                if (control & UART_ERROR)
                    hardware_uart_error = true;

                while ((control & UART_RECEIVE_EMPTY) == 0)
                {
                    push_received_byte(
                        static_cast<uint8_t>(
                            REG_SIODATA8 & 0xFF));

                    control = REG_SIOCNT;

                    if (control & UART_ERROR)
                        hardware_uart_error = true;
                }
            }

            void uart_interrupt_handler()
            {
                drain_hardware_receive_fifo();
            }
        }

        void uart_init()
        {
            hardware_rx_read = 0;
            hardware_rx_write = 0;
            hardware_rx_overflow = false;
            hardware_uart_error = false;

            REG_RCNT = 0;

            // Enter UART mode with FIFO disabled first.
            // This resets the UART FIFO state.
            REG_SIOCNT = UART_MODE;

            IRQ_SetHandler(
                UGBA_IRQ_SERIAL,
                uart_interrupt_handler);

            IRQ_Enable(
                UGBA_IRQ_SERIAL);

            REG_SIOCNT =
                UART_BAUD_115200 |
                UART_8_BITS |
                UART_FIFO_ENABLE |
                UART_SEND_ENABLE |
                UART_RECEIVE_ENABLE |
                UART_MODE |
                UART_IRQ_ENABLE;
        }

        void uart_send(uint8_t byte)
        {
            while (REG_SIOCNT & UART_SEND_FULL)
            {
            }

            REG_SIODATA8 = byte;
        }

        bool uart_receive(uint8_t &byte)
        {
            uint16_t read = hardware_rx_read;

            if (read == hardware_rx_write)
            {
                IRQ_Disable(
                    UGBA_IRQ_SERIAL);

                drain_hardware_receive_fifo();

                IRQ_Enable(
                    UGBA_IRQ_SERIAL);

                read = hardware_rx_read;

                if (read == hardware_rx_write)
                    return false;
            }

            byte = hardware_rx_buffer[read];
            hardware_rx_read = next_hardware_rx_index(read);

            return true;
        }

#endif

    }
}
