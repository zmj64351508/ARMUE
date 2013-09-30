#include "memory_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* If the 2 regions have overlapping area, they are regared as equal.*/
int memory_region_compare(void* a, void* b)
{
	memory_region_t* ma = (memory_region_t*)a;
	memory_region_t* mb = (memory_region_t*)b;
	uint32_t ma_max_addr = ma->base_addr + ma->size - 1;
	uint32_t mb_max_addr = mb->base_addr + mb->size - 1;
	if(ma_max_addr < mb->base_addr){
		return -1;
	}else if(ma->base_addr > mb_max_addr){
		return 1;
	}else{
		return 0;
	}
}

memory_region_t* create_memory_region(void* region_data)
{
	memory_region_t* region = (memory_region_t*)malloc(sizeof(memory_region_t));
	if(region == NULL){
		goto out;
	}
	region->region_data = region_data;
out:
	return region;
}

int add_memroy_region(memory_map_t* memory, memory_region_t* region)
{
	bstree_node_t* new_node = bstree_create_node(region);
	if(new_node == NULL){
		return -ERROR_CREATE;
	}
	bstree_node_t* new_root = bstree_add_node(memory->map, new_node, memory_region_compare);
	if(new_root == NULL){
		return -ERROR_ADD;
	}else{
		/* update the root node as the insert operation may change the root */
		memory->map = new_root;
	}

	return SUCCESS;
}

memory_region_t* find_address(memory_map_t* memory, uint32_t address)
{
	memory_region_t region;
	region.base_addr = address;
	region.size = 1;

	bstree_node_t* node = bstree_find_node(memory->map, &region, memory_region_compare);
	if(node == NULL){
		return NULL;
	}
	return (memory_region_t*)node->data;
}

/* call back function for rom's read */
int general_rom_read(uint32_t addr, uint8_t* buffer, int size, memory_region_t* region)
{
	rom_t* rom = (rom_t*)region->region_data;
	uint32_t offset_addr = addr - region->base_addr;
	uint32_t data32;
	int i;
	switch(size){
	case 4:
		data32 = fetch_rom_data32(addr, rom);
		memcpy(buffer, &data32, 4);
		return 4;
	default:
		for(i = 0; i < size; i++){
			buffer[i] = fetch_rom_data8(offset_addr+i, rom);
			if(buffer[i] == EOF){
				break;
			}
		}
		return i;
	}
}

/* call back function for rom's write */
int general_rom_write(uint32_t addr, uint8_t *buffer, int size, memory_region_t* region)
{
	rom_t *rom = (rom_t*)region->region_data;
	uint32_t offset_addr = addr - region->base_addr;
	int retval, i;
	for(i = 0; i < size; i++){
		retval = send_rom_data8(offset_addr+i, buffer[i], rom);
		if(retval == EOF){
			break;
		}
	}
	return i;
}

int setup_memory_map_rom(memory_map_t* memory, rom_t* rom, int base_addr)
{
	memory_region_t* rom_region = create_memory_region(rom);
	if(rom_region == NULL){
		return ERROR_CREATE;
	}
	rom_region->type = MEMORY_REGION_ROM;
	rom_region->base_addr = base_addr;
	rom_region->size = rom->size;
	rom_region->write = general_rom_write;
	rom_region->read = general_rom_read;
	return add_memroy_region(memory, rom_region);
}

/* The main memory write routine */
int write_memory(uint32_t addr, uint8_t* buffer, int size, memory_map_t* memory)
{
	memory_region_t* region = find_address(memory, addr);
	if(region == NULL){
		LOG(LOG_ERROR, "Can't access address 0x%x\n", addr);
		return -1;
	}

	return region->write(addr, buffer, size, region);
}

/* The main memory read routine */
int read_memory(uint32_t addr, uint8_t* buffer, int size, memory_map_t* memory)
{
	memory_region_t* region = find_address(memory, addr);
	if(region == NULL){
		LOG(LOG_ERROR, "Can't access address 0x%x\n", addr);
		return -1;
	}

	return region->read(addr, buffer, size, region);
}

int general_ram_read(uint32_t addr, uint8_t* buffer, int size, memory_region_t* ram_region)
{
	ram_t* ram = (ram_t*)ram_region->region_data;
	uint32_t offset_addr = addr - ram_region->base_addr;
	memcpy(buffer, ram->data+offset_addr, size);
	return size;
}

int general_ram_write(uint32_t addr, uint8_t* buffer, int size, memory_region_t* ram_region)
{
	ram_t* ram = (ram_t*)ram_region->region_data;
	uint32_t offset_addr = addr - ram_region->base_addr;
	memcpy(ram->data+offset_addr, buffer, size);
	return size;
}

int setup_memory_map_ram(memory_map_t* memory, ram_t* ram, int base_addr)
{
	memory_region_t* ram_region = create_memory_region(ram);
	if(ram_region == NULL){
		return ERROR_CREATE;
	}
	ram_region->type = MEMORY_REGION_RAM;
	ram_region->base_addr = base_addr;
	ram_region->size = ram->size;
	ram_region->write = general_ram_write;
	ram_region->read = general_ram_read;
	return add_memroy_region(memory, ram_region);
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

	/* destory contents */
	// TODO

	free(*map);
	*map = NULL;

	return SUCCESS;
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



