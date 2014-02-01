#include "soc.h"
#include "cpu.h"
#include "module_helper.h"
#include "error_code.h"
#include <stdlib.h>
#include "_types.h"
#include "arm_v7m_ins_decode.h"
#include "cm_system_control_space.h"

static module_t* this_module;
static int registered = 0;

#include <stdio.h>


/****** start the cpu. It will set to cpu->start ******/
int armcm3_startup(cpu_t* cpu)
{
    if(cpu == NULL || cpu->memory_map == NULL){
        return ERROR_NULL_POINTER;
    }

    if(cm_NVIC_startup(cpu) < 0){
        return -ERROR_SOC_STARTUP;
    }

    arm_reg_t *regs = (arm_reg_t *)cpu->regs;

    // set register initial value
    // TODO: Following statements need to be pack in another function which should be in armv7m module
    // reset behaviour refering to B1-642
    // Currently set to a specific value for test
    regs->PC = align_address(get_vector_value(cpu->cm_NVIC, 1));
    regs->R0 = 0x0;
    regs->R1 = 0x000000D9;
    regs->R2 = 0;
    regs->R3 = 0;
    regs->R4 = 0x3456abcd;
    regs->R5 = 0x3456abcd;
    regs->R6 = 0x12345678;
    regs->R7 = 0;
    regs->R8 = 0x0;
    regs->R9 = 0x0;
    regs->R10 = 0x0;
    regs->R11 = 0x0;
    regs->R12 = 0x0;
    SET_REG_VAL_BANKED(regs, SP_INDEX, BANK_INDEX_MSP, get_vector_value(cpu->cm_NVIC, 0));
    SET_REG_VAL_BANKED(regs, SP_INDEX, BANK_INDEX_PSP, 0x10000200);
    regs->xPSR = 0x61000000;
    //SET_EPSR_T(regs, get_vector_value(cpu->cm_NVIC, 1) & BIT_0);
    /* if ESPR T is not zero, refer to B1-625 */

    //regs->LR = 0xFFFFFFFF;
    regs->LR = 0x1FFF0D5F;
    regs->sp_in_use = BANK_INDEX_MSP;

    thumb_state* arm_state = (thumb_state*)cpu->run_info.cpu_spec_info;
    arm_state->mode = MODE_THREAD;

    return SUCCESS;
}

/****** fetch the instruction. It will set to cpu->fetch32 ******/
uint32_t fetch_armcm3_cpu(cpu_t* cpu)
{
    /* always fetch opcode according to PC */
    memory_map_t* memory_map = cpu->memory_map;
    uint32_t addr = ((arm_reg_t*)cpu->regs)->PC;
    uint32_t opcode;
    read_memory(addr, (uint8_t*)&opcode, 4, memory_map);
    return opcode;
}

/* decode instruction, return the instruction info. It will be set to cpu->decode */
ins_t decode_armcm3_cpu(cpu_t* cpu, void* opcode)
{
    ins_t ins_info = {0, NULL, 0};
    uint32_t opcode32 = *(uint32_t*)opcode;

    /* The format of the operation code int the memory is uint16_t little endian. So if it is a 32-bit coded
       operation code, low 16 bit and high 16 bit should be exchaged to fit to uint32_t little endian format */
    if(is_16bit_code(opcode32) == TRUE){
        LOG(LOG_DEBUG, "decode_armcm3_cpu: 16 bit opcode 0x%0x\n", (uint16_t)opcode32);
        ins_info.opcode = opcode32;
        ins_info.excute = thumb_parse_opcode16(opcode32, cpu);
        ins_info.length = 16;
        cpu->run_info.ins_type = ARM_INS_THUMB16;
    }else{
        if(((cm_scs_t *)cpu->system_info)->config.endianess == LITTLE_ENDIAN){
            /* exchange low and high 16bit */
            ins_info.opcode = ((opcode32 >> 16) & 0x0000FFFF) | ((opcode32 << 16) & 0xFFFF0000);
        }
        LOG(LOG_DEBUG, "decode_armcm3_cpu: 32 bit opcode 0x%0x\n", (uint32_t)ins_info.opcode);
        ins_info.excute = thumb_parse_opcode32((uint32_t)ins_info.opcode, cpu);
        ins_info.length = 32;
        cpu->run_info.ins_type = ARM_INS_THUMB32;
    }
    return ins_info;
}

/****** excute the cpu. It will set to cpu->excute ******/
void excute_armcm3_cpu(cpu_t* cpu, ins_t ins_info){

    arm_reg_t *regs = (arm_reg_t *)cpu->regs;
    thumb_state *state = (thumb_state*)cpu->run_info.cpu_spec_info;

    /****** PC always points to the address of next instruction.                */
    /****** when 16bit coded, PC += 2. when 32bit coded, PC += 4.               */
    /****** But if instruction visits PC, it always returns PC+4                */
    armv7m_next_PC(cpu, ins_info.length);
    if(ins_info.length == 16){
        ((thumb_translate16_t)ins_info.excute)((uint16_t)ins_info.opcode, cpu);
    }else{
        // 32bit insturction
        ((thumb_translate32_t)ins_info.excute)((uint32_t)ins_info.opcode, cpu);
    }

    if(!check_and_reset_excuting_IT(state) && InITBlock(regs)){
        ITAdvance(regs);
    }
}

static uint32_t armcm3_get_raw_pc(cpu_t *cpu)
{
    arm_reg_t *regs = (arm_reg_t *)cpu->regs;
    return regs->PC;
}

static void armcm3_set_raw_pc(uint32_t val, cpu_t *cpu)
{
    arm_reg_t *regs = (arm_reg_t *)cpu->regs;
    SET_REG_VAL(regs, PC_INDEX, val);
}

/****** Initialize an instance of the cpu. It will set to module->init_cpu ******/
int init_armcm3_cpu(cpu_t *cpu, soc_conf_t* config)
{
    int retval;

    /* init thumb instrction */
    retval = ins_thumb_init(cpu, config);
    if(retval < 0){
        goto init_ins_fail;
    }

    /* init system control space including MPU, NVIC, SYSTICK, CPUID etc. */
    retval = cm_scs_init(cpu);
    if(retval < 0){
        goto init_scs_fail;
    }

    /* init interfaces */
    cpu->startup = armcm3_startup;
    cpu->fetch32 = fetch_armcm3_cpu;
    cpu->decode = decode_armcm3_cpu;
    cpu->excute = excute_armcm3_cpu;
    cpu->get_raw_pc = armcm3_get_raw_pc;
    cpu->set_raw_pc = armcm3_set_raw_pc;
    set_cpu_module(cpu, this_module);
    cpu->type = CPU_ARM_CM3;

    // add cpu to cpu list
    add_cpu_to_tail(this_module->cpu_list, cpu);

    return SUCCESS;

init_scs_fail:
    ins_thumb_destory(cpu);
init_ins_fail:
    return retval;
}

error_code_t destory_armcm3_cpu(cpu_t** cpu)
{
    ins_thumb_destory(*cpu);
    // delete from cpu list
    delete_cpu(this_module->cpu_list, *cpu);

    dealloc_cpu(cpu);
    return SUCCESS;
}

/****** This is the unregister entrence used by main system ******/
error_code_t unregister_armcm3_module()
{
    if(registered == 0){
        return ERROR_REGISTERED;
    }

    LOG(LOG_DEBUG, "unregister_armcm3_module: unregister arm cortex m3 cpu.\n");
    unregister_module_helper(this_module);

    destory_module(&this_module);

    /* registered is the flag for whether this module is registered or not
       Is there a better way to do something similar ? */
    registered--;

    return SUCCESS;
}

/****** This is the register entrence used by main system ******/
error_code_t register_armcm3_module()
{
    error_code_t error_code;

    /* Important the module can only be registered once
       But this is not a good way. */
    if(registered == 1){
        return ERROR_REGISTERED;
    }

    LOG(LOG_DEBUG, "register_armcm3_module: register arm cortex-m3 cpu.\n");
    // create module
    this_module = create_module();
    if(this_module == NULL){
        return ERROR_CREATE_MODULE;
    }


    // initialize module attributes
    set_module_type(this_module, MODULE_CPU);
    set_module_name(this_module, "arm_cm3");

    // initialize module methods
    set_module_unregister(this_module, unregister_armcm3_module);
    set_module_content_create(this_module, (create_func_t)init_armcm3_cpu);
    set_module_content_destory(this_module, (destory_func_t)destory_armcm3_cpu);

    // register this module
    error_code = register_module_helper(this_module);
    if(error_code != SUCCESS){
        LOG(LOG_ERROR, "register_armcm3_module: can't regist module.\n");
    }

    /* Well, as metioned, this should be improved. */
    registered++;

    return error_code;
}

