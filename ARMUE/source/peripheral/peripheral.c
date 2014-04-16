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

/* dispatch the peripheral event by parsed packet */
int _dispatch_peri_event(pmp_parsed_pkt_t *pkt, peripheral_table_t *peri_table)
{
    int peri_kind = pkt->peri_kind;
    int peri_index = pkt->peri_index;
    int data_kind = pkt->data_kind;
    uint8_t *data = pkt->data;
    int data_len = pkt->data_len;

    // check peri_kind and peri_index
    if(peri_kind >= PERI_MAX_KIND || peri_index >= peri_table[peri_kind].num){
        LOG(LOG_ERROR, "Peripheral with kind: %d, index: %d\n doesn't exsit\n", peri_kind, peri_index);
        return -1;
    }

    int retval;
    // find correspinding peripheral and do the data_process
    peripheral_t *cur_peri = &peri_table[peri_kind].real_peri[peri_index];
    if(cur_peri->data_process == NULL){
        LOG(LOG_ERROR, "data process for the peripheral %d:%d is NULL\n", peri_kind, peri_index);
        return -2;
    }
    retval = cur_peri->data_process(data_kind, data, data_len, cur_peri->user_data);
    if(retval < 0){
        return retval;
    }
    return 0;
}

int dispatch_peri_event(pmp_parsed_pkt_t *pkt)
{
    _dispatch_peri_event(pkt, g_peri_table);
}
