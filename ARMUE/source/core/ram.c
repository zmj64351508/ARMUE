#include "ram.h"
#include <stdlib.h>

ram_t* create_ram(size_t size)
{
    ram_t *ram = (ram_t*)malloc(sizeof(ram_t));
    if(ram == NULL){
        goto ram_null;
    }
    ram->size = size;
    ram->data = (uint8_t*)malloc(size);
    if(ram->data == NULL){
        goto data_null;
    }

    memset(ram->data, 0, size);
    return ram;

data_null:
    free(ram);
ram_null:
    return NULL;
}
