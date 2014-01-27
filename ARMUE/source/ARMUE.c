#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include "module_helper.h"

#include "memory_map.h"
#include "soc.h"
int main(int argc, char **argv)
{
    // register all exsisted modules
    register_all_modules();
    /*
    module_t* uart_module = find_module(_T("uart"));
    peripheral_t* uart = uart_module->create_peripheral();
    uart->reg_num;
    uart->regs;
    uart->mem_base;
    uart->set_operation(0x10, "out,send_message,[31:0]");
    set_memory_map_peripheral(memory_map, uart);
    */
    memory_map_t *memory_map = create_memory_map();

    soc_conf_t soc_conf;
    soc_conf.cpu_num = 1;
    soc_conf.cpu_name = "arm_cm3";
    soc_conf.exception_num = 255;
    soc_conf.nested_level = 10;
    soc_conf.has_GIC = 0;
    soc_conf.memory_map_num = 1;
    soc_conf.memories[0] = memory_map;
    soc_conf.exclusive_high_address = 0xFFFFFFFF;
    soc_conf.exclusive_low_address = 0;

    // ROM
    rom_t* rom = alloc_rom();
    set_rom_size(rom, 0x80000);
    if(SUCCESS != open_rom("E:\\GitHub\\ARMUE\\test.rom", rom)){
        return -1;
    }
    fill_rom_with_bin(rom, "E:\\GitHub\\ARMUE\\cortex_m3_test\\test.bin");
    int result = setup_memory_map_rom(memory_map, rom, 0x00);
    if(result < 0){
        LOG(LOG_ERROR, "Faild to setup ROM\n");
    }

    /* RAM */
    ram_t* ram = create_ram(0x8000);
    result = setup_memory_map_ram(memory_map, ram, 0x10000000);
    if(result < 0){
        LOG(LOG_ERROR, "Failed to setup RAM\n");
    }

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
