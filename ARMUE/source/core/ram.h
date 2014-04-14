#ifndef _RAM_H_
#define _RAM_H_
#ifdef __cplusplus
extern "C"{
#endif

#include "_types.h"
#include <stddef.h>

typedef struct{
    uint32_t size;
    uint8_t* data;
}ram_t;

ram_t* create_ram(size_t size);
int fill_ram_with_bin(ram_t *ram, uint32_t start_addr, char *path);

#ifdef __cplusplus
}
#endif
#endif
