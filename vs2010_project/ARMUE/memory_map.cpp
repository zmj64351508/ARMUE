#include "memory_map.h"
#include <stdio.h>
#include <stdlib.h>


error_code_t set_memory_map_rom(memory_map_t* map, rom_t* rom, int rom_index)
{
	if(map == NULL || rom == NULL){
		return ERROR_NULL_POINTER;
	}

	map->ROM[rom_index] = rom;

	return SUCCESS;
}

error_code_t set_memory_map_ram(memory_map_t* map, ram_t* ram, int ram_index)
{
	if(map == NULL || ram == NULL){
		return ERROR_NULL_POINTER;
	}

	map->RAM[ram_index] = ram;

	return SUCCESS;
}

memory_map_t* create_memory_map()
{
	memory_map_t* map = (memory_map_t*)calloc(1, sizeof(memory_map_t));
	return map;
}


error_code_t destory_memory_map(memory_map_t** map)
{
	if(map == NULL || *map == NULL){
		return ERROR_NULL_POINTER;
	}

	destory_rom(&(*map)->ROM[0]);

	free(*map);
	*map = NULL;

	return SUCCESS;
}


// return the index of the rom which match the address, if it doesn't mach any rom, return -1
int addr_in_rom(uint32_t addr, memory_map_t* map)
{
	if(map == NULL){
		return -1;
	}

	for(int i = 0; i < ROM_MAX; i++){
		if(map->ROM[i]->allocated == TRUE &&
		   addr >= map->ROM[i]->base_address && 
		   addr < map->ROM[i]->base_address + map->ROM[i]->size){
			   return i;
		}
	}

	return -1;
}

// return the index of the ram which match the address, if it doesn't mach any ram, return -1
int addr_in_ram(uint32_t addr, memory_map_t* map)
{
	if(map == NULL){
		return -1;
	}

	for(int i = 0; i < RAM_MAX; i++){
		if(map->RAM[i]->allocated == TRUE &&
			addr >= map->RAM[i]->base_address && 
			addr < map->RAM[i]->base_address + map->RAM[i]->size){
				return i;
		}
	}

	return -1;
}

// set the interrupt table
// NOTE: it doesn't check the array index
error_code_t set_interrput_table(uint32_t* table, uint32_t intrpt_value, int intrpt_num)
{
	if(table == NULL){
		return ERROR_NULL_POINTER;
	}

	table[intrpt_num] = intrpt_value;

	return SUCCESS;
}
