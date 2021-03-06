#include "memory_map.h"
#include "peripheral.h"
#include "core_connect.h"
#include "uart.h"
#include "ARMUE.h"

#define LPC1768_UART0_BASE 0x4000C000
#define LPC1768_UART0_SIZE 0x34
#define LPC1768_UART_BUFFER_LEN 16

typedef struct lpc1768_uart_t
{
    uart_t generic_uart;
    int index;
}lpc1768_uart_t;

lpc1768_uart_t lpc1768_uart0;
//lpc1768_uart_t lpc1768_uart2;
//lpc1768_uart_t lpc1768_uart3;

/* this will be called when some UART data is received */
int lpc1768_uart_data_process(int data_kind, uint8_t *data, unsigned int len, void *user_data)
{
    LOG(LOG_DEBUG, "UART recved:\n");
    lpc1768_uart_t *uart = (lpc1768_uart_t *)user_data;
    switch(data_kind){
    case PMP_DATA_KIND_DENERIC:
        uart_store_in_buffer(&uart->generic_uart, data, len);
    default:
        break;
    }
    return 0;
}

/* peripheral_t is the structure for peripheral register */
peripheral_t peri_lpc1768_uart0 = {
    .user_data = &lpc1768_uart0,
    .data_process = lpc1768_uart_data_process,
};

/* All the register read and write function */
void URBR(uint8_t *buffer, int rw_flag, lpc1768_uart_t *uart)
{
    int result;
    if(rw_flag == MEM_READ){
        // TODO: some other operation to corresponding PE FE and BI bits
        result = uart_read_data(&uart->generic_uart, buffer);
        if(result < 0){
            *buffer = 0;
        }
    }else{
        // read only
    }
}

void UTHR(uint8_t *buffer, int rw_flag, lpc1768_uart_t *uart)
{
    if(rw_flag == MEM_READ){
        // write only
    }else{
        uart_send_byte(armue_get_peri_connect(), uart->index, buffer);
    }
}

#define GET_DLAB() 0

// the register access runtine
// TODO: is there some better way?
#define LPC1768_UART_REGS_ACCESS(offset, buffer, rw_flag, user_defined_access, region_data)\
switch(offset){\
case 0x00:\
    if(GET_DLAB() == 0){\
        URBR(buffer, rw_flag, region_data);\
    }else{\
        /*UDLL(buffer, rw_flag, region_data);*/\
    }\
    break;\
case 0x04:\
    if(GET_DLAB() == 0){\
        UTHR(buffer, rw_flag, region_data);\
    }else{\
        /*UDLM(buffer, rw_flag, region_data);*/\
    }\
    break;\
}\

int lpc1768_uart_read(uint32_t offset, uint8_t *buffer, int size, memory_region_t *region)
{
    lpc1768_uart_t *uart = (lpc1768_uart_t *)region->region_data;
    LPC1768_UART_REGS_ACCESS(offset, buffer, MEM_READ, NULL, uart);
    return 4;
}

int lpc1768_uart_write(uint32_t offset, uint8_t *buffer, int size, memory_region_t *region)
{
    lpc1768_uart_t *uart = (lpc1768_uart_t *)region->region_data;
    LPC1768_UART_REGS_ACCESS(offset, buffer, MEM_WRITE, NULL, uart);
    return 4;
}

/* initialize lpc1768 uart */
int lpc1768_uart_init(cpu_t *cpu)
{
    int retval;
    // get memory first
    memory_map_t *memory = cpu->memory_map;
    if(memory == NULL){
        retval = -ERROR_MEMORY_MAP;
        goto no_memory;
    }

    // request memory region
    memory_region_t *region_uart0 = request_memory_region(memory, LPC1768_UART0_BASE, LPC1768_UART0_SIZE);
    if(region_uart0 == NULL){
        retval = -ERROR_MEMORY_MAP;
        goto get_region_fail;
    }

    // initialize custom data
    uart_init(&lpc1768_uart0.generic_uart, LPC1768_UART_BUFFER_LEN);
    lpc1768_uart0.index = 0;

    // set memory region interfaces
    region_uart0->region_data = &lpc1768_uart0;
    region_uart0->read = lpc1768_uart_read;
    region_uart0->write = lpc1768_uart_write;
    region_uart0->type = MEMORY_REGION_PERI;

    /* request for listening to the input */
    request_peripheral(PERI_UART, 3);
    register_peripheral(PERI_UART, 0, &peri_lpc1768_uart0);
    return 0;

get_region_fail:
no_memory:
    return retval;
}

