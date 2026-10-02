
#include "bn_common.h"
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

        volatile uint16_t &REG_SIOCNT = *reinterpret_cast<volatile uint16_t *>(0x04000128);

        volatile uint16_t &REG_SIODATA8 = *reinterpret_cast<volatile uint16_t *>(0x0400012A);

        volatile uint16_t &REG_RCNT = *reinterpret_cast<volatile uint16_t *>(0x04000134);

        static constexpr uint16_t UART_BAUD_115200 = 3;

        static constexpr uint16_t UART_SEND_FULL = 1 << 4;

        static constexpr uint16_t UART_RECEIVE_EMPTY = 1 << 5;

        static constexpr uint16_t UART_8_BITS = 1 << 7;

        static constexpr uint16_t UART_SEND_ENABLE = 1 << 10;

        static constexpr uint16_t UART_RECEIVE_ENABLE = 1 << 11;

        static constexpr uint16_t UART_MODE = (1 << 12) | (1 << 13);

        void uart_init()
        {
            REG_RCNT = 0;

            REG_SIOCNT = UART_BAUD_115200 |
                         UART_8_BITS |
                         UART_SEND_ENABLE |
                         UART_RECEIVE_ENABLE |
                         UART_MODE;
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
            if (REG_SIOCNT & UART_RECEIVE_EMPTY)
                return false;

            byte = static_cast<uint8_t>(REG_SIODATA8 & 0xFF);

            return true;
        }

#endif

    }
}