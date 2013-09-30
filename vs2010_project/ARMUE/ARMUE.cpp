// ARMUE.cpp : 定义控制台应用程序的入口点。
//
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include "module_helper.h"

#include "memory_map.h"
#include "soc.h"
int _tmain(int argc, _TCHAR* argv[])
{
	// register all exsisted modules
	register_all_modules();
	
	memory_map_t *memory_map = create_memory_map();		
	// ROM
	rom_t* rom = alloc_rom();
	set_rom_size(rom, 0x1000);
	if(SUCCESS != open_rom(_T("E:\\GitHub\\ARMUE\\vs2010_project\\test.rom"), rom)){
		return -1;
	}
	fill_rom_with_bin(rom, _T("E:\\GitHub\\ARMUE\\vs2010_project\\cortex_m3_test\\test.bin"));
	int result = setup_memory_map_rom(memory_map, rom, 0x00);
	if(result < 0){
		LOG(LOG_ERROR, "Faild to setup ROM\n");
	}

	/* RAM */
	ram_t* ram = create_ram(0x1000);
	result = setup_memory_map_ram(memory_map, ram, 0x01000000);
	if(result < 0){
		LOG(LOG_ERROR, "Failed to setup RAM\n");
	}

	/*
	module_t* uart_module = find_module(_T("uart"));
	peripheral_t* uart = uart_module->create_peripheral();
	uart->reg_num;
	uart->regs;
	uart->mem_base;
	uart->set_operation(0x10, "out,send_message,[31:0]");
	set_memory_map_peripheral(memory_map, uart);
	*/

	soc_conf_t soc_conf;
	soc_conf.cpu_num = 1;
	soc_conf.cpu_name = "arm_cm3";
	soc_conf.memory_map_num = 1;
	soc_conf.memories[0] = memory_map;

	// soc
	uint32_t opcode;
	soc_t* soc = create_soc(&soc_conf);
	if(soc != NULL){
		startup_soc(soc);
		while(1){
			opcode = run_soc(soc);
			if(opcode == 0)
				break;
		}
		destory_soc(&soc);
	}

	unregister_all_modules();
}