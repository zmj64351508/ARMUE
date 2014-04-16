#ifndef _UART_H_
#define _UART_H_
#ifdef __cplusplus
extern "C"{
#endif

#define UART_STOP_1   1
#define UART_STOP_1_5 2
#define UART_STOP_2   3

#define UART_VALID_ODD  1
#define UART_VALID_EVEN 2

#include "fifo.h"
#include <stdint.h>
#include "core_connect.h"

typedef struct uart_t
{
    uint8_t stop_bit;
    uint8_t valid_type;
    uint8_t data_len;
    int baud;
    fifo_t *in_buffer;
}uart_t;

#pragma pack(1)
struct uart_config_pkt_t{
    uint32_t baud;
    uint8_t stop_bit;
    uint8_t valid_type;
    uint8_t data_len;
};
#pragma pack()

int uart_init(uart_t *uart, int buf_len);
int uart_store_in_buffer(uart_t *uart, uint8_t *data, int len);
void uart_send_data(core_connect_t *connect, int index, void *data, int len);
void uart_send_byte(core_connect_t *connect, int index, void *data);
int uart_read_data(uart_t *uart, void *buffer);

#ifdef __cplusplus
}
#endif
#endif
