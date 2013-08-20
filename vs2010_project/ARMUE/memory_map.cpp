#include "memory_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


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
		if(map->ROM[i] && map->ROM[i]->allocated == TRUE &&
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
		if(map->RAM[i] && map->RAM[i]->allocated == TRUE &&
			addr >= map->RAM[i]->base_address && 
			addr < map->RAM[i]->base_address + map->RAM[i]->size){
				return i;
		}
	}

	return -1;
}

uint32_t get_from_memory32(uint32_t addr, memory_map_t* memory_map)
{
	int storer_index;
	
	if((storer_index = addr_in_rom(addr, memory_map)) >= 0){
		return fetch_rom_data32(addr, memory_map->ROM[storer_index]);
	}else if((storer_index = addr_in_ram(addr, memory_map)) >= 0){
		LOG(LOG_DEBUG, "fetch from ram\n");
		return 0;// TODO: fetch from ram
	}else{
		LOG(LOG_ERROR, "get_from_memory32: Can't access 0x%x\n", addr);
		return -1;
	}
}

int get_from_memory_general(uint32_t addr, int size, uint8_t* buffer, memory_map_t* memory_map)
{
	int storer_index;
	if((storer_index = addr_in_rom(addr, memory_map)) >= 0){
		for(int i = 0; i < size; i++){
			buffer[i] = fetch_rom_data8(addr+i, memory_map->ROM[storer_index]);
		}
		return 0;
	}else if((storer_index = addr_in_ram(addr, memory_map)) >= 0){
		LOG(LOG_DEBUG, "fetch from ram\n");
		return 0; //TODO: fetch from ram
	}else{
		LOG(LOG_ERROR, "get_from_memory32: Can't access 0x%x\n", addr);
		return -1;
	}
}

int get_from_memory(uint32_t addr, int size, uint8_t* buffer, memory_map_t* memory_map)
{
	assert(buffer != NULL);
	uint32_t data32;
	int retval;
	switch(size){
	case 4:
		data32 = get_from_memory32(addr, memory_map);
		memcpy(buffer, &data32, 4);
		retval = 0;
		break;
	default:
		retval = get_from_memory_general(addr, size, buffer, memory_map);
	}
	return retval;
}

int set_to_memory_general(uint32_t addr, int size, uint8_t* buffer, memory_map_t* memory_map)
{
	int retval;
	int storer_index;
	if((storer_index = addr_in_rom(addr, memory_map)) >= 0){
		for(int i = 0; i < size; i++){
			retval = send_rom_data8(addr+i, buffer[i], memory_map->ROM[storer_index]);
			if(retval < 0){
				LOG(LOG_ERROR, "set_to_memroy_general: Write cause error\n");
				return i;
			}
		}
		return 0;
	}else if((storer_index = addr_in_ram(addr, memory_map)) >= 0){
		LOG(LOG_DEBUG, "set to ram\n");
		return 0; //TODO: set to ram
	}else{
		LOG(LOG_ERROR, "set_to_memory_general: Can't access 0x%x\n", addr);
		return -1;
	}
}

int set_to_memory(uint32_t addr, int size, uint8_t* buffer, memory_map_t* memory_map)
{
	assert(buffer != NULL);
	int retval = set_to_memory_general(addr, size, buffer, memory_map);
	return retval;
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
