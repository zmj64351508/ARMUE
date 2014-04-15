#include "uart.h"

int uart_init(uart_t *uart, int buf_len)
{
    uart->in_buffer = create_fifo(buf_len, sizeof(uint8_t));
    return 0;
}

int uart_pack_data(uint8_t *output, void *input, int data_len, uint8_t stop_bit, uint8_t valid_type, int baud)
{
    // should make proper packet for uart
    *output = *(uint8_t *)input;
    return 1;
}

int uart_unpack_data()
{
    return 0;
}

void uart_send_data(core_connect_t *connect, int index, void *data, int data_len, int stop_bit, int valid_type, int baud)
{
    uint8_t tosend[10];
    int len = uart_pack_data(tosend, data, data_len, stop_bit, valid_type, baud);
    make_pmp_data_packet(connect, PERI_UART, index, PMP_DATA_KIND_DENERIC, tosend, len);
    pmp_send(connect);
}

void uart_read_data(uart_t *uart, void *buffer)
{
    fifo_out(uart->in_buffer, buffer);
}

