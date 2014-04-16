#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include "module_helper.h"
#include "arm_gdb_stub.h"
#include "getopt.h"

#include "memory_map.h"
#include "soc.h"
#include "config.h"

#include "windows.h"
#include "core_connect.h"

#include "lpc1768_uart.h"

// connect to peripheral monitor
static core_connect_t *g_peri_connect;

const char short_options[] = "hgc:";
const struct option long_options[] = {
    {"help",    no_argument,        NULL,   'h'},
    {"gdb",     no_argument,        NULL,   'g'},
    {"client",  required_argument,  NULL,   'c'},
    {0, 0, 0, 0},
};


/* for other part of the program that want to know the connect to the peripheral */
core_connect_t *armue_get_peri_connect()
{
    return g_peri_connect;
}

int main(int argc, char **argv)
{
    char c;
    int option_index;
    while(1){
        c = getopt_long(argc, argv, short_options, long_options, &option_index);
        if(c == -1){
            break;
        }
        switch(c){
        case 'h':
            printf("help\n");
            return 0;
        case 'g':
            config.gdb_debug = TRUE;
            break;
        case 'c':
            config.client = TRUE;
            config.pipe_name = (char *)malloc(strlen(optarg));
            strcpy(config.pipe_name, optarg);
            break;
        default:
            printf("Try --help");
            return 0;
        }
    };

    int retval;
    if(config.client){
        g_peri_connect = create_core_connect(1024, config.pipe_name);
        retval = connect_monitor(g_peri_connect);
        if(retval != SUCCESS){
            return -1;
        }
    }

    // register all exsisted modules
    register_all_modules();

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
    fill_rom_with_zero(rom);
    //fill_rom_with_bin(rom, 0, "E:\\GitHub\\ARMUE\\cortex_m3_test\\test.bin");
    fill_rom_with_bin(rom, 0, "E:\\GitHub\\ARMUE\\svc_fsm_m3_test\\test.bin");
    //fill_rom_with_bin(rom, "E:\\LPC11U3X_demo_board\\software\\_OK_systick\\test.bin");
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
    if(soc == NULL){
        return -1;
    }
    if(config.gdb_debug){
        soc->stub = create_stub();
        if(soc->stub == NULL){
            return -1;
        }
    }

    lpc1768_uart_init(soc->cpu[0]);

    // main loop for emulation
    startup_soc(soc);
    if(config.gdb_debug){
        init_stub(soc->stub);
    }
    while(1){
        opcode = run_soc(soc);
        //getchar();
        if(opcode == 0)
            break;
    }
    destory_soc(&soc);

    unregister_all_modules();
    return 0;
}
