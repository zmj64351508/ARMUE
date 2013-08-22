#ifndef _ROM_H_
#define _ROM_H_
#include <stdio.h>
#include <tchar.h>
#include "_types.h"
#include "error_code.h"

#define READ_BASE_ADDRESS -1
#define READ_ROM_SIZE -1

#define ROM_READ 0
#define ROM_WRITE 1

typedef struct rom_s_t
{
	bool_t allocated;			// the flag whether the rom is allocated to specific rom file
	int content_start;			// where the rom data start.
	int last_offset;			// last read or write offset of the rom file
	FILE* rom_file;				
	uint32_t base_address;		// base address of the rom in memory
	uint32_t size;				// the size of the rom in byte
	int rw_flag;
}rom_t;

rom_t* alloc_rom();
error_code_t destory_rom(rom_t** rom);
error_code_t open_rom(_I _TCHAR* path, _IO rom_t* rom);
error_code_t fill_rom_with_bin(rom_t *rom, _TCHAR* bin_path);

error_code_t set_rom_size(rom_t *rom, uint32_t size);
error_code_t set_rom_base_addr(rom_t* rom, uint32_t base_addr);

int send_rom_data8(uint32_t addr, uint8_t char_to_send, rom_t* rom);
uint8_t fetch_rom_data8(uint32_t addr, rom_t* rom);
uint32_t fetch_rom_data32(uint32_t addr, rom_t* rom);
uint16_t fetch_rom_data16(uint32_t addr, rom_t* rom);
error_code_t fill_rom_with_zero(rom_t *rom);
#endif