#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "module_helper.h"

#include "memory_map.h"
#include "soc.h"

int main(int argc, char **argv)
{
        // register all exsisted modules
    register_all_modules();

    memory_map_t *memory_map = create_memory_map();
    // ROM
    rom_t* rom = alloc_rom();
    set_rom_size(rom, 0x1000);
    if(SUCCESS != open_rom("E:\\GitHub\\ARMUE\\test.rom", rom)){
        return -1;
    }
    fill_rom_with_bin(rom, "E:\\GitHub\\ARMUE\\cortex_m3_test\\test.bin");
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

    soc_conf_t soc_conf;
    soc_conf.cpu_num = 1;
    soc_conf.cpu_name = "arm_cm3";
    soc_conf.exception_num = 255;
    soc_conf.nested_level = 10;
    soc_conf.has_GIC = 0;
    soc_conf.memory_map_num = 1;
    soc_conf.memories[0] = memory_map;

    // soc
    uint32_t opcode;
    soc_t* soc = create_soc(&soc_conf);
    if(soc != NULL){
        startup_soc(soc);

        /* timer start */
        clock_t run_begin, run_end;
        run_begin = clock();

        while(1){
            opcode = run_soc(soc);
            if(opcode == 0 || soc->cpu[0]->cycle == 5000000)
                break;
        }

        /* timer end */
        run_end = clock();
        double ips = (double)soc->cpu[0]->cycle / ((double)(run_end - run_begin) / 1000);
        printf("cycles: %d\n", soc->cpu[0]->cycle);
        printf("time  : %fms\n", (double)(run_end - run_begin) / 1000);
        printf("ips is %f\n", ips);

        destory_soc(&soc);
    }

    unregister_all_modules();
}
