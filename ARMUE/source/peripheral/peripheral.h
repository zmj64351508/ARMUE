#ifndef _PERIPHERAL_H_
#define _PERIPHERAL_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>
#include "soc.h"

enum peripheral_kind_t{
    PERI_UNKNOW = -1,
    PERI_I2C = 0,
    PERI_UART,
    PERI_MAX_KIND,
};

typedef struct peripheral_t{
    void *user_data;
    int (*data_process)(uint8_t *data, unsigned int len, void *user_data);
}peripheral_t;

typedef struct peripheral_table_t{
    int num;
    peripheral_t *real_peri;
}peripheral_table_t;

int request_peripheral(int peri_kind, int peri_amount);
int register_peripheral(int peri_kind, int peri_index, peripheral_t *peri_data);

extern peripheral_table_t g_peri_table[];

#ifdef __cplusplus
}
#endif

#endif
