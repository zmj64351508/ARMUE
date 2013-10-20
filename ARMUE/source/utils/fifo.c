#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "fifo.h"

fifo_t* create_fifo(size_t fifo_length, size_t data_size)
{
    fifo_t* fifo = (fifo_t*)malloc(sizeof(fifo));
    if(fifo == NULL){
        goto fifo_null;
    }

    fifo->data = (void*)calloc(fifo_length, data_size);
    if(fifo->data == NULL){
        goto data_null;
    }
    fifo->in_index = 0;
    fifo->out_index = 0;
    fifo->empty = 1;
    fifo->length = fifo_length;
    fifo->data_size = data_size;
    return fifo;

data_null:
    free(fifo);
fifo_null:
    return NULL;
}

void destory_fifo(fifo_t** fifo)
{
    fifo_t* destory = *fifo;
    free(destory->data);
    free(destory);
    *fifo = NULL;
}

int fifo_in(fifo_t* fifo, void *in_data)
{
    if(fifo->in_index == fifo->out_index && !fifo->empty){
        return -1;
    }
    uint8_t *data = (uint8_t*)fifo->data;
    int index = fifo->in_index * fifo->data_size; 
    uint8_t *addr = data + index;
    memcpy(addr, in_data, fifo->data_size);
    fifo->in_index = (fifo->in_index + 1)%fifo->length;
    fifo->empty = 0;

    return 0;
}

int fifo_out(fifo_t* fifo, void *out_data)
{
    if(fifo->empty){
        return -1;
    }
    uint8_t *data = (uint8_t*)fifo->data;
    int index = fifo->out_index * fifo->data_size;
    uint8_t *addr = data + index;
    memcpy(out_data, addr, fifo->data_size);
    fifo->out_index = (fifo->out_index +1) % fifo->length;
    if(fifo->in_index == fifo->out_index){
        fifo->empty = 0;
    }
    return 0;
}

