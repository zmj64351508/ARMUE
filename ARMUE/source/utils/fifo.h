#ifndef _FIFO_H_
#define _FIFO_H_
#ifdef __cplusplus
extern "C"{
#endif

#include <stddef.h>

typedef struct fifo_t{
    void *data;
    size_t data_size;
    size_t length;
    int empty;
    int in_index;
    int out_index;
}fifo_t;

#define fifo_empty(fifo_ptr) ((fifo_ptr)->empty)

fifo_t* create_fifo(size_t fifo_length, size_t data_size);
void destory_fifo(fifo_t** fifo);
int fifo_in(fifo_t* fifo, void *in_data);
int fifo_out(fifo_t* fifo, void *out_data);
int peek_fifo(fifo_t* fifo, void *out_data);

#ifdef __cplusplus
}
#endif
#endif
