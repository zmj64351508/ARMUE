#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include "module_helper.h"

#include "memory_map.h"
#include "soc.h"
#include "arm_v7m_ins_decode.h"
#include "arm_gdb_stub.h"
#include "config.h"
enum state_t{
    STATE_START = 1,
    STATE_REG,
    STATE_ASSIGN,
    STATE_VALUE,
    STATE_END,
    STATE_ONCE_END,
};

enum reg_index{
    INDEX_XPSR = 20,
    INDEX_PSP,
    INDEX_MSP,
    INDEX_CONTROL,
    INDEX_FAULTMASK,
    INDEX_BASEPRI,
    INDEX_PRIMASK,
};

typedef struct reg_pair_t{
    char name[10];
    uint32_t value;
}reg_pair_t;

#define CHAR_TO_INT(c) (int)((c) - 0x30)
int char_to_hex(char c)
{
    if(c >= '0' && c <= '9'){
        return CHAR_TO_INT(c);
    }else if(c >= 'A' && c <= 'F'){
        return c - 0x41 + 0xA;
    }else if(c >= 'a' && c <= 'f'){
        return c - 0x61 + 0xA;
    }else{
        return -1;
    }
}

/* start state */
int start_parse(int event, reg_pair_t *reg_pair)
{
    switch(event){
    case ' ':
        return STATE_START;
    default:
        reg_pair->name[0] = event;
        return STATE_REG;
    }
}

int reg_parse(int event, reg_pair_t *reg_pair)
{
    static int i;
    switch(event){
    case ' ':
    case '=':
        reg_pair->name[++i] = '\0';
        i = 0;
        return STATE_ASSIGN;
    default:
        reg_pair->name[++i] = event;
        return STATE_REG;
    }
}

int assign_parse(int event, reg_pair_t *reg_pair)
{
    if((event >= '0' && event <= '9') || (event >= 'A' && event <= 'F')){
        reg_pair->value = char_to_hex(event);
        return STATE_VALUE;
    }
    switch(event){
    case ' ':
    case '=':
        return STATE_ASSIGN;
    default:
        return -1;
    }
}

int value_parse(int event, reg_pair_t *reg_pair)
{
    if((event >= '0' && event <= '9') || (event >= 'A' && event <= 'F')){
        reg_pair->value *= 0x10;
        reg_pair->value += char_to_hex(event);
        return STATE_VALUE;
    }

    switch(event){
    case ',':
    case '\n':
    case '\r':
        return STATE_END;
    default:
        return -1;
    }
}

int end_parse(int event, reg_pair_t *reg_pair, arm_reg_t *regs)
{
    if(strcmp("R0", reg_pair->name) == 0){
        regs->R[0] = reg_pair->value;
    }else if(strcmp("R1", reg_pair->name) == 0){
        regs->R[1]= reg_pair->value;
    }else if(strcmp("R2", reg_pair->name) == 0){
        regs->R[2]= reg_pair->value;
    }else if(strcmp("R3", reg_pair->name) == 0){
        regs->R[3]= reg_pair->value;
    }else if(strcmp("R4", reg_pair->name) == 0){
        regs->R[4]= reg_pair->value;
    }else if(strcmp("R5", reg_pair->name) == 0){
        regs->R[5]= reg_pair->value;
    }else if(strcmp("R6", reg_pair->name) == 0){
        regs->R[6]= reg_pair->value;
    }else if(strcmp("R7", reg_pair->name) == 0){
        regs->R[7]= reg_pair->value;
    }else if(strcmp("R8", reg_pair->name) == 0){
        regs->R[8]= reg_pair->value;
    }else if(strcmp("R9", reg_pair->name) == 0){
        regs->R[9]= reg_pair->value;
    }else if(strcmp("R10", reg_pair->name) == 0){
        regs->R[10]= reg_pair->value;
    }else if(strcmp("R11", reg_pair->name) == 0){
        regs->R[11]= reg_pair->value;
    }else if(strcmp("R12", reg_pair->name) == 0){
        regs->R[12]= reg_pair->value;
    }else if(strcmp("R13", reg_pair->name) == 0){
        regs->R[13]= reg_pair->value;
    }else if(strcmp("R14(LR)", reg_pair->name) == 0){
        regs->R[14]= reg_pair->value;
    }else if(strcmp("R15(PC)", reg_pair->name) == 0){
        regs->R[15]= reg_pair->value;
    }else if(strcmp("MSP", reg_pair->name) == 0){
        regs->SP_bank[BANK_INDEX_MSP] = reg_pair->value;
        if(reg_pair->value == regs->SP){
            regs->sp_in_use = BANK_INDEX_MSP;
        }
    }else if(strcmp("PSP", reg_pair->name) == 0){
        regs->SP_bank[BANK_INDEX_PSP] = reg_pair->value;
        if(reg_pair->value == regs->SP){
            regs->sp_in_use = BANK_INDEX_PSP;
        }
    }else if(strcmp("XPSR", reg_pair->name) == 0){
        regs->xPSR = reg_pair->value;
    }else if(strcmp("CONTROL", reg_pair->name) == 0){
        regs->CONTROL = reg_pair->value >> 24;
    }else if(strcmp("BASEPRI", reg_pair->name) == 0){
        regs->BASEPRI = reg_pair->value >> 8;
    }else if(strcmp("FAULTMASK", reg_pair->name) == 0){
        regs->FAULTMASK = reg_pair->value >> 16;
    }else if(strcmp("PRIMASK", reg_pair->name) == 0){
        regs->PRIMASK = reg_pair->value;
        return STATE_ONCE_END;
    }

    return start_parse(event, reg_pair);
}

int parse_reg_engine(int state, char event, reg_pair_t *reg_pair, arm_reg_t *regs)
{
    switch(state){
    case STATE_START:
        return start_parse(event, reg_pair);
    case STATE_REG:
        return reg_parse(event, reg_pair);
    case STATE_ASSIGN:
        return assign_parse(event, reg_pair);
    case STATE_VALUE:
        return value_parse(event, reg_pair);
    case STATE_END:
        return end_parse(event, reg_pair, regs);
    default:
        return -1;
    }
}

int parse_reg_file_once(FILE *file, arm_reg_t *regs)
{
    char c;
    int current_state = STATE_START;
    reg_pair_t reg_pair;
    do{
        c = getc(file);
        if(feof(file)){
            return EOF;
        }
        current_state = parse_reg_engine(current_state, c, &reg_pair, regs);
    }while(current_state != STATE_ONCE_END && current_state >= 0);

    if(current_state < 0){
        fprintf(stderr, "error occurred\n");
    }

    return 0;
}

#define CHECK_DISMATCH_REG(sim, parsed, reg_name)\
if((sim)->reg_name != (parsed)->reg_name){\
    printf("Dismatch:"#reg_name", 0x%x should be 0x%x\n", (sim)->reg_name, (parsed)->reg_name);\
    retval = -1;\
}

int compare_arm_regs(arm_reg_t *sim_reg, arm_reg_t *parsed_reg)
{
    int i;
    int retval = 0;
    for(i = 0; i < 13; i++){
        if(sim_reg->R[i] != parsed_reg->R[i]){
            printf("Dismatch: R%d, 0x%x should be 0x%x\n", i, sim_reg->R[i], parsed_reg->R[i]);
            retval = -1;
        }
    }
    CHECK_DISMATCH_REG(sim_reg, parsed_reg, SP);
    CHECK_DISMATCH_REG(sim_reg, parsed_reg, LR);
    CHECK_DISMATCH_REG(sim_reg, parsed_reg, PC);

    CHECK_DISMATCH_REG(sim_reg, parsed_reg, xPSR);
    CHECK_DISMATCH_REG(sim_reg, parsed_reg, FAULTMASK);
    CHECK_DISMATCH_REG(sim_reg, parsed_reg, PRIMASK);
    CHECK_DISMATCH_REG(sim_reg, parsed_reg, BASEPRI);
    CHECK_DISMATCH_REG(sim_reg, parsed_reg, CONTROL);
    //CHECK_DISMATCH_REG(sim_reg, parsed_reg, SP_bank[BANK_INDEX_MSP]);
    //CHECK_DISMATCH_REG(sim_reg, parsed_reg, SP_bank[BANK_INDEX_PSP]);
    CHECK_DISMATCH_REG(sim_reg, parsed_reg, sp_in_use);

    return retval;
}

int main(int argc, char **argv)
{
    // register all exsisted modules
    register_all_modules();
    memory_map_t *memory_map = create_memory_map();

    config.gdb_debug = FALSE;

    // the SOC configuration
    soc_conf_t soc_conf;
    soc_conf.cpu_num = 1;
    soc_conf.cpu_name = "arm_cm3";
    soc_conf.exception_num = 255;
    soc_conf.nested_level = 10;
    soc_conf.has_GIC = FALSE;
    soc_conf.memory_map_num = 1;
    soc_conf.memories[0] = memory_map;
    soc_conf.exclusive_high_address = 0xFFFFFFFF;
    soc_conf.exclusive_low_address = 0;

    // ROM
    rom_t* rom = alloc_rom();
    set_rom_size(rom, 0x80000);
    if(SUCCESS != open_rom("../../test.rom", rom)){
        return -1;
    }
    fill_rom_with_bin(rom, "../../test_example/test.bin");
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

    /* prepare for the test */
    FILE *reg_file = fopen("../../test_example/ins_test.txt", "r");
    if(reg_file == NULL){
        return -1;
    }
    arm_reg_t parsed_regs;
    arm_reg_t *sim_regs;
    int ret, comp_result;
    uint32_t execute_pc;

    // soc
    soc_t* soc = create_soc(&soc_conf);
    soc->stub = create_stub();
    if(soc->stub == NULL){
        return -1;
    }
    if(soc != NULL){
        startup_soc(soc);
        if(config.gdb_debug){
            init_stub(soc->stub);
        }

        /* copy the initial state to simulated registers */
        sim_regs = ARMv7m_GET_REGS(soc->cpu[0]);
        parse_reg_file_once(reg_file, &parsed_regs);
        memcpy(sim_regs, &parsed_regs, sizeof(arm_reg_t));

        /* test routine */
        while(1){
            ret = parse_reg_file_once(reg_file, &parsed_regs);
            if(ret == EOF){
                break;
            }
            execute_pc = sim_regs->PC;
            run_soc(soc);
            comp_result = compare_arm_regs(sim_regs, &parsed_regs);
            printf("%s\n", comp_result == 0 ? "OK": "FAIL");
            if(comp_result != 0){
                printf("current pc is 0x%x\n", execute_pc);
                printf("press any key to continue and ignore the failure\n");
                getchar();
                memcpy(sim_regs, &parsed_regs, sizeof(arm_reg_t));
            }
        }


        /* when a dismatch case found, you can manually set the register value and debug the instruction */
//        while(1){
//            if(ret == EOF){
//                break;
//            }
//
//            sim_regs->PC = 0x14A;
//            sim_regs->xPSR = 0x21000000;
//
//            run_soc(soc);
//        }

        destory_soc(&soc);
    }

    unregister_all_modules();

    return 0;
}
