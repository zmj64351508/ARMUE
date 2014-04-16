#include "uart.h"

int uart_init(uart_t *uart, int buf_len)
{
    if(uart->in_buffer != NULL){
        return -1;
    }

    uart->in_buffer = create_fifo(buf_len, sizeof(uint8_t));
    return 0;
}

/* Send configuration to other side. Generally be called when the configuration is changed
   so that the other side of the program can respond to the change.*/
void uart_send_config(core_connect_t *connect, uart_t *uart, int index)
{
    struct uart_config_pkt_t config_data;
    config_data.baud = uart->baud;
    config_data.data_len = uart->data_len;
    config_data.stop_bit = uart->stop_bit;
    config_data.valid_type = uart->valid_type;
    make_pmp_data_packet(connect, PERI_UART, index, PMP_DATA_KIND_PROTOCOL, (uint8_t *)&config_data, sizeof(config_data));
    pmp_send(connect);
}

void uart_parse_config_pkt(uint8_t *buffer, uart_t *result)
{
    struct uart_config_pkt_t *config_pkt = (struct uart_config_pkt_t *)buffer;
    result->baud = config_pkt->baud;
    result->data_len = config_pkt->data_len;
    result->stop_bit = config_pkt->stop_bit;
    result->valid_type = config_pkt->valid_type;
}

int uart_store_in_buffer(uart_t *uart, uint8_t *data, int len)
{
    int i, result;
    for(i = 0; i < len; i++){
        result = fifo_in(uart->in_buffer, data + i);
        if(result < 0){
            return result;
        }
    }
    return 0;
}

/* Send data without the configuration information of the UART */
void uart_send_data(core_connect_t *connect, int index, void *data, int len)
{
    make_pmp_data_packet(connect, PERI_UART, index, PMP_DATA_KIND_DENERIC, data, 1);
    pmp_send(connect);
}

void uart_send_byte(core_connect_t *connect, int index, void *data)
{
    uart_send_data(connect, index, data, 1);
}

int uart_read_data(uart_t *uart, void *buffer)
{
    return fifo_out(uart->in_buffer, buffer);
}

