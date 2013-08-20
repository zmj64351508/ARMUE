#ifndef _MEMORY_MAP_H_
#define _MEMORY_MAP_H_

#include "_types.h"
#include "error_code.h"
#include "ram.h"
#include "rom.h"

#define MEM_MAP_CACHE_SIZE 4000
#define MAX_INTERRUPT_NUM 255
#define ROM_MAX 4
#define RAM_MAX 4
typedef struct  
{
	uint32_t size_total;
	
	// interrput table
	uint32_t interrupt_table[MAX_INTERRUPT_NUM];
	
	// storage
	rom_t* ROM[ROM_MAX];
	ram_t* RAM[RAM_MAX];
	uint8_t map_cache[MEM_MAP_CACHE_SIZE];
}memory_map_t;


error_code_t set_memory_map_rom(memory_map_t* map, rom_t* rom, int rom_index);
error_code_t set_memory_map_ram(memory_map_t* map, ram_t* ram, int ram_index);
error_code_t set_interrput_table(uint32_t* memory_map, uint32_t intrpt_value, int intrpt_num);
memory_map_t* create_memory_map();
error_code_t destory_memory_map(memory_map_t** map);

//int addr_in_rom(uint32_t addr, memory_map_t* map);
//int addr_in_ram(uint32_t addr, memory_map_t* map);
int get_from_memory(uint32_t addr, int size, uint8_t* buffer, memory_map_t* memory_map);
uint32_t get_from_memory32(uint32_t addr, memory_map_t* memory_map);
int set_to_memory(uint32_t addr, int size, uint8_t* buffer, memory_map_t* memory_map);
#endif