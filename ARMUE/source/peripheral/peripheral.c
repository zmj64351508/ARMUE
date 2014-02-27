#include "peripheral.h"
#include "error_code.h"
#include <string.h>

// peripheral table contains all the information of the registered peripherals
peripheral_table_t g_peri_table[PERI_MAX_KIND];

// requeset the amount of such peripheral
int _request_peripheral(peripheral_table_t *table, int peri_kind, int peri_amount)
{

    if(peri_kind >= PERI_MAX_KIND){
        LOG(LOG_ERROR, "register_peripheral: unknow peripheral kind\n");
        return -1;
    }
    peripheral_t *new_peri = (peripheral_t *)calloc(peri_amount, sizeof(peripheral_t));
    if(NULL == new_peri){
        return -2;
    }
    int i;
    // if the table is already existed, realloc it and copy the content to the new location
    if(table[peri_kind].num != 0){
        for(i = 0; i < table[peri_kind].num; i++){
            memcpy(&new_peri[i], &table[peri_kind].real_peri[i], sizeof(peripheral_t));
        }
        free(table[peri_kind].real_peri);
    }

    table[peri_kind].real_peri = new_peri;
    table[peri_kind].num += peri_amount;
    return 0;
}

int request_peripheral(int peri_kind, int peri_amount)
{
    return _request_peripheral(g_peri_table, peri_kind, peri_amount);
}

int _register_peripheral(peripheral_table_t *table, int peri_kind, int peri_index, peripheral_t *peri_data)
{
    if(table == NULL || peri_data == NULL){
        return -1;
    }
    if(peri_kind >= PERI_MAX_KIND){
        LOG(LOG_ERROR, "register_peripheral: unknow peripheral kind\n");
        return -2;
    }
    if(peri_index >= table[peri_kind].num){
        LOG(LOG_ERROR, "register_peripheral: peripheral index too large\n");
    }
    memcpy(&table[peri_kind].real_peri[peri_index], peri_data, sizeof(peripheral_t));
    return 0;
}

int register_peripheral(int peri_kind, int peri_index, peripheral_t *peri_data)
{
    return _register_peripheral(g_peri_table, peri_kind, peri_index, peri_data);
}
