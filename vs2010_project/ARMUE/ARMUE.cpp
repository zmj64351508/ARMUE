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
//	printf("%d\n",sizeof(short int));
	// register all exsisted modules
	register_all_modules();

	// memory map
	rom_t* rom = alloc_rom();
	set_rom_size(rom, 2000);
	set_rom_base_addr(rom, 0x00);
	if(SUCCESS != open_rom(_T("E:\\GitHub\\ARMUE\\vs2010_project\\test.rom"), rom)){
		return -1;
	}
	fill_rom_with_bin(rom, _T("E:\\GitHub\\ARMUE\\vs2010_project\\cortex_m3_test\\test.bin"));
	memory_map_t *memory_map = create_memory_map();		
	set_memory_map_rom(memory_map, rom, 0);

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
	uint32_t ins;
	soc_t* soc = create_soc(&soc_conf);
	if(soc != NULL){
		startup_soc(soc);
		for(;;){
			ins = run_soc(soc);
			if(ins == 0)
				break;
		}
		destory_soc(&soc);
	}
		
	unregister_all_modules();
}