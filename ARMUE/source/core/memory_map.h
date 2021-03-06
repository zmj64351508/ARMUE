#ifndef _MEMORY_MAP_H_
#define _MEMORY_MAP_H_
#ifdef __cplusplus
extern "C"{
#endif

#include "_types.h"
#include "error_code.h"
#include "ram.h"
#include "rom.h"

typedef enum{
    MEMORY_REGION_UNKNOW,
    MEMORY_REGION_ROM,
    MEMORY_REGION_RAM,
    MEMORY_REGION_SYS,
    MEMORY_REGION_PERI,
}memory_region_type_t;

#define MEM_MAP_CACHE_SIZE 4000
#define ROM_MAX 4
#define RAM_MAX 4

#define MEM_READ    1
#define MEM_WRITE   2

#include "bstree.h"
typedef struct{
    uint32_t size_total;
    bstree_node_t *map;
}memory_map_t;

typedef struct memory_region_t{
    uint32_t base_addr;
    uint32_t size;
    memory_region_type_t type;
    void *region_data;
    int (*read)(uint32_t offset, uint8_t *buffer, int size, struct memory_region_t *region);
    int (*write)(uint32_t offset, uint8_t *buffer, int size, struct memory_region_t *region);
}memory_region_t;


int setup_memory_map_rom(memory_map_t* memory, rom_t* rom, int base_addr);
int setup_memory_map_ram(memory_map_t* memory, ram_t* ram, int base_addr);
memory_map_t* create_memory_map();
error_code_t destory_memory_map(memory_map_t** map);

//int addr_in_rom(uint32_t addr, memory_map_t* map);
//int addr_in_ram(uint32_t addr, memory_map_t* map);
memory_region_t* find_memory_region(memory_map_t* memory, uint32_t address, int size);
#define find_address(memory, address) find_memory_region(memory, address, 1)
memory_region_t* request_memory_region(memory_map_t *memory, uint32_t address, int size);

int read_memory(uint32_t addr, uint8_t* buffer, int size, memory_map_t* memory);
int write_memory(uint32_t addr, uint8_t* buffer, int size, memory_map_t* memory);

#ifdef __cplusplus
}
#endif
#endif
