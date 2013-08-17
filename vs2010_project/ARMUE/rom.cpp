#include "rom.h"
#include <string.h>
#include <stdlib.h>

rom_t* alloc_rom()
{
	LOG(LOG_DEBUG, "create_rom\n");
	rom_t* rom = (rom_t *)calloc(1, sizeof(rom_t));
	
	return rom;
}

error_code_t destory_rom(rom_t** rom)
{
	LOG(LOG_DEBUG, "destory_rom\n");
	if(rom == NULL || *rom == NULL){
		return ERROR_NULL_POINTER;
	}

	if((*rom)->rom_file != NULL){
		fclose((*rom)->rom_file);
	}

	free(*rom);
	*rom = NULL;

	return SUCCESS;
}

error_code_t clear_rom(rom_t* rom)
{
	if(rom == NULL){
		return ERROR_NULL_POINTER;
	}

	memset(rom, 0, sizeof(rom_t));

	return SUCCESS;
}


void copy_whole_file(FILE* src, FILE* dst)
{
	char src_c;

	src_c = fgetc(src);
	while(!feof(src)){		
		fputc(src_c, dst);
		src_c = fgetc(src);
	}

	fflush(dst);
}


error_code_t fill_rom_with_zero(rom_t *rom)
{
	if(rom == NULL){
		return ERROR_NULL_POINTER;
	}

	for(uint32_t i = 0; i < rom->size; i++){
		fputc(0, rom->rom_file);
	}

	fflush(rom->rom_file);
	return SUCCESS;
}

error_code_t fill_rom_with_bin(rom_t *rom, _TCHAR* bin_path)
{
	if(rom == NULL || bin_path == NULL){
		return ERROR_NULL_POINTER;
	}

	FILE* bin_file = _wfopen(bin_path, _T("rb"));

	if(bin_file != NULL){
		fseek(rom->rom_file, rom->content_start, SEEK_SET);
		copy_whole_file(bin_file, rom->rom_file);	
		rom->last_offset = ftell(rom->rom_file) - 1;

		fclose(bin_file);
		return SUCCESS;

	}else{
		LOG(LOG_ERROR, "fill_rom_with_bin: Can't open %ws\n", bin_path);
		return ERROR_INVALID_PATH;
	}

}


static int parse_next_line_int(FILE* file, char* string_start)
{
	int parsed_int = 0;
	char c;
	char parsed_int_string[100] = {0};

	int i = 0;
	do{
		c = getc(file);
		parsed_int_string[i] = c;
		i++;
	}while(c != '\n' && !feof(file) && i < 100);

	int string_len = strlen(string_start);
	if(strncmp(string_start, parsed_int_string, string_len) == 0){
		parsed_int = atoi(&parsed_int_string[string_len]);
	}

	return parsed_int;
}

static error_code_t validate_rom_param(rom_t* rom)
{
	if(rom == NULL){
		return ERROR_NULL_POINTER;
	}

	if(rom->base_address){

	}

	if(rom->size == 0){
		return ERROR_INVALID_ROM_FILE;
	}

	return SUCCESS;
}

static error_code_t parse_rom_file_head(_IO rom_t *rom)
{
	if(rom == NULL){
		return ERROR_NULL_POINTER;
	}
	if(rom->rom_file == NULL){
		return ERROR_NULL_POINTER;
	}
	
	FILE *file = rom->rom_file;

	uint32_t read_base_address;
	uint32_t read_size;

	read_size = (uint32_t)parse_next_line_int(file, "size:");
	read_base_address = (uint32_t)parse_next_line_int(file, "base_address:");

	if(read_size == READ_ROM_SIZE){
		rom->size = read_size;
	}else if(read_size != rom->size){
		return ERROR_ROM_PARAM_DISMATCH;
	}

	if(read_base_address == READ_BASE_ADDRESS){
		rom->base_address = read_base_address;
	}else if(read_base_address != rom->base_address){
		return ERROR_ROM_PARAM_DISMATCH;
	}

	return validate_rom_param(rom);
}


/*
 * @brief: if rom file exsists, open the rom. Else use the parameter "rom" to create the rom file
 * @parameter:	path - the path of rom file													
 *				rom - if the rom file exisits, this is an output value. Else, we use this
 *					  to create the rom file. In other word, this value didn't work at all
 *					  when the file exsists.
 * @return:		SUCCESS, ERROR_INVALID_PARAM, ERROR_INVALID_ROM_FILE, ERROR_CREATE			
 */
error_code_t open_rom(_I _TCHAR* path, _IO rom_t* rom)
{
	LOG(LOG_DEBUG, "open_rom: %ws\n", path);
	if(rom == NULL || path == NULL){
		return ERROR_NULL_POINTER;
	}

	// rom file exists, open it
	if(_waccess(path, 0) != -1){
		rom->rom_file = _wfopen(path, _T("rb+"));
		
		// TODO: 
		//if(validate_rom_file() == SUCCESS){
		//	do something
		//}else{
		//	return fault
		//}

		// get rom size ,base address and set other attributes
		if(parse_rom_file_head(rom) == SUCCESS){
			rom->content_start = ftell(rom->rom_file);
			rom->last_offset = rom->content_start-1;
			rom->allocated = TRUE;
			return SUCCESS;
		}else{
			fclose(rom->rom_file);
			clear_rom(rom);
			return ERROR_INVALID_ROM_FILE;
		}

	// rom file doesn't exsist, create it
	}else{
		rom->rom_file = _wfopen(path, _T("w+"));
		if(rom->rom_file != NULL ){
			
			// edit the rom file and set rom attributes
			if(validate_rom_param(rom) == SUCCESS){
				fprintf(rom->rom_file, "size:%d\n", rom->size);
				fprintf(rom->rom_file, "base_address:%d\n", rom->base_address);
				rom->content_start = ftell(rom->rom_file);
				rom->last_offset = rom->content_start-1;
				rom->allocated = TRUE;
				fill_rom_with_zero(rom);
				return SUCCESS;
			}else{
				fclose(rom->rom_file);
				clear_rom(rom);
				return ERROR_CREATE_ROM_FILE;
			}
		}else{
			return ERROR_CREATE_ROM_FILE;
		}
	}
}

error_code_t set_rom_size(rom_t *rom, uint32_t size)
{
	if(rom == NULL){
		return ERROR_NULL_POINTER;
	}

	rom->size = size;

	return SUCCESS;
}

error_code_t set_rom_base_addr(rom_t* rom, uint32_t base_addr)
{
	if(rom == NULL){
		return ERROR_NULL_POINTER;
	}

	rom->base_address = base_addr;

	return SUCCESS;
}

// get the 32bit data in rom specifical address
uint32_t fetch_rom_data32(uint32_t addr, rom_t* rom)
{
	int cur_offset = addr - rom->base_address + rom->content_start;
	int8_t buf[4] = {0};
	
	if(cur_offset == rom->last_offset + 1){
		for(int i = 0; i < 4; i++){
			buf[i] = fgetc(rom->rom_file);
		}
		//fgets(buf, 4, rom->rom_file);
		rom->last_offset += 4;
	}else{
		fseek(rom->rom_file, cur_offset, SEEK_SET);
		for(int i = 0; i < 4; i++){
			buf[i] = fgetc(rom->rom_file);
		}
		//fgets(buf, 4, rom->rom_file);
		rom->last_offset = cur_offset+3;
	}
	
	return *(uint32_t*)buf;
}

uint16_t fetch_rom_data16(uint32_t addr, rom_t* rom)
{
	int cur_offset = addr - rom->base_address + rom->content_start;
	int8_t buf[2] = {0};

	if(cur_offset == rom->last_offset + 1){
		for(int i = 0; i < 2; i++){
			buf[i] = getc(rom->rom_file);
		}
		//fgets(buf, 4, rom->rom_file);
		rom->last_offset += 2;
	}else{
		fseek(rom->rom_file, cur_offset, SEEK_SET);
		for(int i = 0; i < 2; i++){
			buf[i] = getc(rom->rom_file);
		}
		//fgets(buf, 4, rom->rom_file);
		rom->last_offset = cur_offset+1;
	}

	return *(uint32_t*)buf;
}
