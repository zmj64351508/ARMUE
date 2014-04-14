#include "ram.h"
#include "error_code.h"
#include <stdio.h>
#include <string.h>
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

int copy_file_to_buffer(char* buffer, size_t max_size, FILE* file)
{
    char c = getc(file);
    int i = 0;
    while(!feof(file) && i < max_size){
        buffer[i++] = c;
        c = fgetc(file);
    }

    return i;
}

int fill_ram_with_bin(ram_t *ram, uint32_t start_addr, char *path)
{
    if(ram == NULL || path == NULL){
        return -1;
    }
    if(ram->size == 0 || ram->data == NULL){
        return -2;
    }

    FILE* bin_file = fopen(path, "rb");

    if(bin_file != NULL){
        fseek(bin_file, start_addr, SEEK_SET);
        copy_file_to_buffer((char*)ram->data, ram->size, bin_file);
        fclose(bin_file);
        return 0;

    }else{
        LOG(LOG_ERROR, "fill_rom_with_bin: Can't open %s\n", path);
        fclose(bin_file);
        return -3;
    }
}
