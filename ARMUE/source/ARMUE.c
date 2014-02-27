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

// connect to peripheral monitor
core_connect_t *g_peri_connect;

const char short_options[] = "hgc:";
const struct option long_options[] = {
    {"help",    no_argument,        NULL,   'h'},
    {"gdb",     no_argument,        NULL,   'g'},
    {"client",  required_argument,  NULL,   'c'},
    {0, 0, 0, 0},
};

int i2c_test(uint8_t *data, unsigned int len, void *user_data)
{
    LOG(LOG_DEBUG, "i2c recved:\n");
    int i;
    for(i = 0; i < len; i++){
        putchar(data[i]);
    }
    putchar('\n');
    return 0;
}
int a = 2;
peripheral_t i2c = {
    .user_data = &a,
    .data_process = i2c_test,
};

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
//    fill_rom_with_bin(rom, "E:\\GitHub\\ARMUE\\cortex_m3_test\\test.bin");
    //fill_rom_with_bin(rom, "E:\\GitHub\\ARMUE\\svc_fsm_m3_test\\test.bin");
    fill_rom_with_bin(rom, "E:\\LPC11U3X_demo_board\\software\\_OK_systick\\test.bin");
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

    /* test for peripheral monitor */
    request_peripheral(PERI_I2C, 2);
    register_peripheral(PERI_I2C, 0, &i2c);
    // connected
    while(config.client){
        bool_t has_input = check_monitor_input(g_peri_connect);
        if(has_input){
            parse_monitor_input(g_peri_connect, g_peri_table);
            g_peri_connect->recv_buf[g_peri_connect->recv_len] = '\0';
            printf("%s\n", g_peri_connect->recv_buf);
        }
//            send_peripheral_output_direct(g_peri_connect, "hello", 6);
//            Sleep(1);
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
