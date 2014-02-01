#include "cpu.h"
#include "arm_v7m_ins_decode.h"
#include "cm_NVIC.h"
#include "cm_system_control_space.h"
#include <stdlib.h>

enum cm_NVIC_prio{
    CM_NVIC_PRIO_RESET        =    -3,
    CM_NVIC_PRIO_NMI        =    -2,
    CM_NVIC_PRIO_HARDFAULT    =    -1,
    CM_NVIC_PRIO_UNKNOW        =     0,
};

enum cm_NVIC_vector{
    CM_NVIC_VEC_RESET        =    1,
    CM_NVIC_VEC_NMI            =    2,
    CM_NVIC_VEC_HARDFAULT    =    3,
    CM_NVIC_VEC_MEMMANFAULT    =    4,
    CM_NVIC_VEC_BUSFAULT    =    5,
    CM_NVIC_VEC_USAGEFAULT    =    6,
    CM_NVIC_VEC_SVCALL        =    11,
    CM_NVIC_VEC_DEBUG        =    12,
    CM_NVIC_VEC_PENDSV        =    14,
    CM_NVIC_VEC_SYSTICK        =    15,
};

/* ARMv7-M defined operation */
inline void DeActivate(int exc_num, cpu_t *cpu)
{
    cm_NVIC_t *NVIC_info = (cm_NVIC_t *)cpu->cm_NVIC->controller_info;
    arm_reg_t *regs = ARMv7m_GET_REGS(cpu);
    NVIC_info->exception_active[exc_num] = 0;
    NVIC_info->nested_exception--;
    if(GET_IPSR(regs) != 0x2ul){
        regs->FAULTMASK &= ~1ul;
    }
}

uint32_t ReturnAddress(int excep_num, cpu_t *cpu)
{
    arm_reg_t *regs = ARMv7m_GET_REGS(cpu);
    switch(excep_num){
    case CM_NVIC_VEC_HARDFAULT:
    case CM_NVIC_VEC_BUSFAULT:
    case CM_NVIC_VEC_USAGEFAULT:
        /* address of current instruction */
        return GET_REG_VAL(regs, PC_INDEX) - 4;
    default:
        /* address of next instruction */
        return GET_REG_VAL(regs, PC_RAW_INDEX);
    }
}

void PushStack(int excep_num, cpu_t *cpu)
{
    arm_reg_t *regs = ARMv7m_GET_REGS(cpu);
    thumb_state *state = ARMv7m_GET_STATE(cpu);

    int framesize;
    uint32_t forcealign = 0;
    /*
    if(HaveFPExt() && (exc_return & 0x10ul) == 0){
        framesize = 0x68;
        forcealign = 1;
    }else{*/
        framesize = 0x20;
        forcealign = 1;
        //forcealign = CCR.STKALIGN;
    /*}*/

    uint32_t spmask = ~(forcealign << 2);
    uint32_t frameptralign;
    uint32_t frameptr;
    uint32_t banked_sp;

    sync_banked_register(regs, SP_INDEX);

    if(GET_CONTROL_SPSEL(regs) == 1 && state->mode == MODE_THREAD){
        banked_sp = BANK_INDEX_PSP;
        regs->sp_in_use = BANK_INDEX_PSP;
    }else{
        banked_sp = BANK_INDEX_MSP;
        regs->sp_in_use = BANK_INDEX_MSP;
    }
    frameptralign = (regs->SP_bank[banked_sp] >> 2) & forcealign;
    regs->SP_bank[banked_sp] = (regs->SP_bank[banked_sp] - framesize) & spmask;
    frameptr = regs->SP_bank[banked_sp];

    uint32_t regval;
    regval = GET_REG_VAL(regs, 0);
    MemA(frameptr, 4, (uint8_t*)&regval, MEM_WRITE, cpu);
    regval = GET_REG_VAL(regs, 1);
    MemA(frameptr + 0x4, 4, (uint8_t*)&regval, MEM_WRITE, cpu);
    regval = GET_REG_VAL(regs, 2);
    MemA(frameptr + 0x8, 4, (uint8_t*)&regval, MEM_WRITE, cpu);
    regval = GET_REG_VAL(regs, 3);
    MemA(frameptr + 0xC, 4, (uint8_t*)&regval, MEM_WRITE, cpu);
    regval = GET_REG_VAL(regs, 12);
    MemA(frameptr + 0x10, 4, (uint8_t*)&regval, MEM_WRITE, cpu);
    regval = GET_REG_VAL(regs, LR_INDEX);
    MemA(frameptr + 0x14, 4, (uint8_t*)&regval, MEM_WRITE, cpu);
    regval = ReturnAddress(excep_num, cpu);
    MemA(frameptr + 0x18, 4, (uint8_t*)&regval, MEM_WRITE, cpu);
    regval = (GET_PSR(regs) & ~(1ul << 9)) | (frameptralign << 9);
    MemA(frameptr + 0x1C, 4, (uint8_t*)&regval, MEM_WRITE, cpu);

    restore_banked_register(regs, SP_INDEX);
    /* HaveFPExt() && CONTROL.FPCA == 1*/

    /* HaveFPExt()*/

    /*else*/
    if(state->mode == MODE_HANDLER){
        SET_REG_VAL(regs, LR_INDEX, 0xFFFFFFF1);
    }else{
        SET_REG_VAL(regs, LR_INDEX, 0xFFFFFFF9 | (GET_CONTROL_SPSEL(regs) << 2));
    }
}

void ExceptionTaken(int excep_num, cpu_t *cpu)
{
    arm_reg_t *regs = ARMv7m_GET_REGS(cpu);
    thumb_state *state = ARMv7m_GET_STATE(cpu);
    cm_NVIC_t *NVIC_info = (cm_NVIC_t *)cpu->cm_NVIC->controller_info;

    int ret = fifo_in(state->cur_exception, &excep_num);
    if(ret < 0){
        LOG(LOG_WARN, "ExceptionTaken: can't add exception #%d\n", excep_num);
        return;
    }

    uint32_t addr = get_vector_value(cpu->cm_NVIC, excep_num);
    SET_REG_VAL(regs, PC_INDEX, addr & 0xFFFFFFFE);
    uint32_t tbit = addr & 1ul;
    state->mode = MODE_HANDLER;
    SET_EPSR_T(regs, tbit);
    SET_IPSR(regs, excep_num & 0x1FF);
    SET_ITSTATE(regs, 0);
    /*
    if(haveFPExt){
    }
    */
    SET_CONTROL_SPSEL(regs, 0);
    NVIC_info->exception_active[excep_num] = 1;
    NVIC_info->nested_exception++;

    // SCS_UpdateStatusRegs
    // ClearExclusiveLocal
    // SetEventRegister
    // Barrier
}

void PopStack(uint32_t frameptr, int exc_return, cpu_t* cpu)
{
    arm_reg_t *regs = ARMv7m_GET_REGS(cpu);

    int framesize;
    uint8_t forcealign;
    /*
    if(HaveFPExt() && (exc_return & 0x10ul) == 0){
        framesize = 0x68;
        forcealign = 1;
    }else{*/
        framesize = 0x20;
        forcealign = 1;
        //forcealign = CCR.STKALIGN;
    /*}*/

    uint32_t regval;
    MemA(frameptr, 4, (uint8_t*)&regval, MEM_READ, cpu);
    SET_REG_VAL(regs, 0, regval);
    MemA(frameptr + 0x4, 4, (uint8_t*)&regval, MEM_READ, cpu);
    SET_REG_VAL(regs, 1, regval);
    MemA(frameptr + 0x8, 4, (uint8_t*)&regval, MEM_READ, cpu);
    SET_REG_VAL(regs, 2, regval);
    MemA(frameptr + 0xC, 4, (uint8_t*)&regval, MEM_READ, cpu);
    SET_REG_VAL(regs, 3, regval);
    MemA(frameptr + 0x10, 4, (uint8_t*)&regval, MEM_READ, cpu);
    SET_REG_VAL(regs, 12, regval);
    MemA(frameptr + 0x14, 4, (uint8_t*)&regval, MEM_READ, cpu);
    SET_REG_VAL(regs, LR_INDEX, regval);
    MemA(frameptr + 0x18, 4, (uint8_t*)&regval, MEM_READ, cpu);
    SET_REG_VAL(regs, PC_INDEX, regval);
    uint32_t psr;
    MemA(frameptr + 0x1C, 4, (uint8_t*)&psr, MEM_READ, cpu);

/*    if(HaveFPExt()){

    }
    */

    uint32_t spmask = ((psr >> 9) & forcealign) << 2;
    switch(exc_return & 0xFul){
    case 0x1:
    case 0x9:
        regs->SP_bank[BANK_INDEX_MSP] = (regs->SP_bank[BANK_INDEX_MSP] + framesize) | spmask;
        break;
    case 0xD:
        regs->SP_bank[BANK_INDEX_PSP] = (regs->SP_bank[BANK_INDEX_PSP] + framesize) | spmask;
    }
    /*
    if(HaveDSPExt()){
        SET_PSR(regs, psr);
    }else{*/
        uint32_t cur_psr = GET_PSR(regs);
        cur_psr &= ~0xFF00FFFF;
        psr &= 0xFF00FFFF;
        SET_PSR(regs, cur_psr | psr);
    /*}*/
}

void ExceptionReturn(uint32_t exc_return, cpu_t *cpu)
{
    thumb_state *state = ARMv7m_GET_STATE(cpu);
    arm_reg_t *regs = ARMv7m_GET_REGS(cpu);
    cm_NVIC_t *NVIC_info = (cm_NVIC_t *)cpu->cm_NVIC->controller_info;

    /*
    if HaveFPExt(){
        if((exc_return & 0x00FFFFE0) != 0x00FFFFE0) unpredictable
    }else{
        if((exc_return & 0x00FFFFF0) != 0x00FFFFF0) unpredictable
    }
    */
    int ret_excep_num;
    int ret = fifo_out(state->cur_exception, &ret_excep_num);
    if(ret < 0){
        LOG(LOG_WARN, "ExceptionReturn: unknow exception returned\n");
        return;
    }

    uint8_t nested_activation = NVIC_info->nested_exception;
    if(NVIC_info->exception_active[ret_excep_num] == 0){
        DeActivate(ret_excep_num, cpu);
        goto usage_fault;
        return;
    }

    sync_banked_register(regs, SP_INDEX);

    uint32_t framptr;
    switch(exc_return & 0xFul){
    case 0x1:
        framptr = regs->SP_bank[BANK_INDEX_MSP];
        regs->sp_in_use = BANK_INDEX_MSP;
        state->mode = MODE_HANDLER;
        SET_CONTROL_SPSEL(regs, 0);
        break;
    case 0x9:
        if(nested_activation != 1 /*&& CCR.NONBASETHRDENA == 0*/){
            DeActivate(ret_excep_num, cpu);
            goto usage_fault;
        }else{
            framptr = regs->SP_bank[BANK_INDEX_MSP];
            regs->sp_in_use = BANK_INDEX_MSP;
            state->mode = MODE_THREAD;
            SET_CONTROL_SPSEL(regs, 0);
        }
        break;
    case 0xD:
        if(nested_activation != 1 /*&& CCR.NONBASETHRDENA == 0*/){
            DeActivate(ret_excep_num, cpu);
            goto usage_fault;
            return;
        }else{
            framptr = regs->SP_bank[BANK_INDEX_PSP];
            regs->sp_in_use = BANK_INDEX_PSP;
            state->mode = MODE_THREAD;
            SET_CONTROL_SPSEL(regs, 1);
        }
        break;
    default:
        DeActivate(ret_excep_num, cpu);
        goto usage_fault;
    }

    DeActivate(ret_excep_num, cpu);
    PopStack(framptr, exc_return, cpu);

    if((state->mode == MODE_HANDLER && (GET_IPSR(regs) & 0x1FFul) == 0) ||
        (state->mode == MODE_THREAD && (GET_IPSR(regs) & 0x1FFul) != 0)){
        PushStack(ret_excep_num, cpu);
        goto usage_fault;
    }

    //ClearExclusiveLocal();
    //SetEventRegister();
    //Barrier();

    restore_banked_register(regs, SP_INDEX);
    return;

usage_fault:
    restore_banked_register(regs, SP_INDEX);
    // UFSR.INVPC = 1;
    SET_REG_VAL(regs, LR_INDEX, 0xF0000000 + exc_return);
    cpu->cm_NVIC->throw_exception(CM_NVIC_VEC_USAGEFAULT, cpu->cm_NVIC);
    return;
}

int ExecutionPriority(cpu_t *cpu)
{
    arm_reg_t *regs = ARMv7m_GET_REGS(cpu);
    thumb_state *state = ARMv7m_GET_STATE(cpu);
    cm_scs_t *scs = (cm_scs_t *)cpu->system_info;

    int cur_excep;
    int highest_pri = 256;
    int boosted_pri = 256;
    int ret = peek_fifo(state->cur_exception, &cur_excep);
    if(ret < 0){
        return highest_pri;
    }

    vector_exception_t *vector_excep = cpu->cm_NVIC;
    highest_pri = vector_excep->prio_table[cur_excep];

    int group_val = 0x2ul << scs->config.prigroup;
    int sub_group_val = highest_pri % group_val;
    highest_pri -= sub_group_val;

    uint32_t basepri = GET_BASEPRI(regs);
    if(LOW_BIT32(basepri, 8) != 0){
        boosted_pri = LOW_BIT32(basepri, 8);
        sub_group_val = boosted_pri % group_val;
        boosted_pri -= sub_group_val;
    }

    if(LOW_BIT32(GET_PRIMASK(regs), 1) == 1){
        boosted_pri = 0;
    }

    if(LOW_BIT32(GET_FAULTMASK(regs), 1) == 1){
        boosted_pri = -1;
    }

    int priority;
    if(boosted_pri < highest_pri){
        priority = boosted_pri;
    }else{
        priority = highest_pri;
    }

    return priority;
}
/* ARMv7-M defined operation end */

rom_t* find_startup_rom(memory_map_t* memory)
{
    memory_region_t* region = find_address(memory, 0);
    if(region == NULL || region->type != MEMORY_REGION_ROM){
        return NULL;
    }
    return (rom_t*)region->region_data;
}

inline int cm_NVIC_get_preempt_proi(int cm_prio, cm_NVIC_t* info)
{
    return (cm_prio & info->prio_mask) & info->preempt_mask;
}

/* Vector table initialization */
void cm_NVIC_vector_table_init(vector_exception_t *controller, memory_map_t *memory)
{
    uint32_t addr = 0;
    uint32_t interrput_vector = 0;
    int inpterrupt_table_size = controller->vector_table_size;
    int i;
    for(i = 0; i < inpterrupt_table_size; i++){
        read_memory(addr, (uint8_t*)&interrput_vector, 4, memory);
        set_vector_table(controller, interrput_vector, i);
        addr += 4;
    }
}

/* The fixed priority table initialization */
void cm_NVIC_prio_table_init(vector_exception_t *controller)
{
    int i;
    for(i = 0; i < controller->vector_table_size; i++){
        controller->prio_table[i] = CM_NVIC_PRIO_UNKNOW;
    }
    controller->prio_table[CM_NVIC_VEC_RESET]        = CM_NVIC_PRIO_RESET;
    controller->prio_table[CM_NVIC_VEC_NMI]            = CM_NVIC_PRIO_NMI;
    controller->prio_table[CM_NVIC_VEC_HARDFAULT]    = CM_NVIC_PRIO_HARDFAULT;
}

cm_NVIC_t* create_cm_NVIC_info()
{
    cm_NVIC_t *info = (cm_NVIC_t*)malloc(sizeof(cm_NVIC_t));
    if(info == NULL){
        goto info_null;
    }
    return info;

info_null:
    return NULL;
}

void destory_cm_NVIC_info(cm_NVIC_t **info)
{
    free(*info);
    info = NULL;
}

int setup_cm_NVIC_info(vector_exception_t* controller, Input cpu_t *cpu)
{
    cm_NVIC_t* info = (cm_NVIC_t*)controller->controller_info;

    /* pending list */
    info->pending_list = bheap_create(controller->vector_table_size, sizeof(uint32_t));
    if(info->pending_list == NULL){
        goto pending_list_null;
    }

    /* other NVIC variable */
    info->preempt_mask = 0xF;
    info->prio_mask = 0xF;
    info->nested_exception = 0;
    info->interrupt_lines = cpu->cm_NVIC->vector_table_size / 32;
    int i;
    for(i = 0; i < NVIC_MAX_EXCEPTION; i++){
        info->exception_active[i] = 0;
    }
    return 0;

pending_list_null:
    return -1;
}

/* startup when cpu starts */
int cm_NVIC_startup(cpu_t *cpu)
{
    if(cpu == NULL || cpu->memory_map == NULL){
        return -ERROR_NULL_POINTER;
    }

    memory_map_t* memory_map = cpu->memory_map;

    // search for start rom which base address is 0x00
    rom_t *startup_rom = find_startup_rom(memory_map);
    if(startup_rom == NULL){
        return -ERROR_NO_START_ROM;
    }

    cm_NVIC_vector_table_init(cpu->cm_NVIC, memory_map);
    cm_NVIC_prio_table_init(cpu->cm_NVIC);

    return SUCCESS;
}

/* init with the cpu creation, returns the created cm_NVIC_t */
vector_exception_t* cm_NVIC_init(cpu_t* cpu)
{
    if(cpu == NULL){
        return NULL;
    }

    /* create and setup NVIC private data */
    cm_NVIC_t *info = create_cm_NVIC_info();
    cpu->cm_NVIC->controller_info = info;
    if(info == NULL){
        goto create_info_fail;
    }
    if(setup_cm_NVIC_info(cpu->cm_NVIC, cpu) < 0){
        goto setup_info_fail;
    }

    /* setup exception interfaces */
    cpu->cm_NVIC->throw_exception = cm_NVIC_throw_exception;
    cpu->cm_NVIC->check_exception = cm_NVIC_check_exception;
    cpu->cm_NVIC->handle_exception = cm_NVIC_handle_exception;

    return cpu->cm_NVIC;

setup_info_fail:
    destory_cm_NVIC_info((cm_NVIC_t**)&cpu->cm_NVIC->controller_info);
create_info_fail:
    return NULL;
}

int cm_NVIC_throw_exception(int vector_num, struct vector_exception_t* controller)
{
    cm_NVIC_t* NVIC_info = (cm_NVIC_t*)controller->controller_info;
    int prio = controller->prio_table[vector_num];
    int mod_prio = (prio << 9) | (vector_num & 0xFFul);
    return bheap_insert(NVIC_info->pending_list, &mod_prio, bheap_compare_int_smaller);
}

int cm_NVIC_check_exception(cpu_t* cpu)
{
    cm_NVIC_t* NVIC_info = (cm_NVIC_t*)cpu->cm_NVIC->controller_info;
    int mod_prio;

    /* peek the first pending exception if it exsits */
    int retval = bheap_peek_top(NVIC_info->pending_list, &mod_prio);
    if(retval < 0){
        return 0;
    }

    /* mod_prio is defined as: exception number | (priority << 9)*/
    int prio = mod_prio >> 9;
    thumb_state* state = ARMv7m_GET_STATE(cpu);
    int cur_exception;
    peek_fifo(state->cur_exception, &cur_exception);

    /* If in handler, check preempt priority for the preemption */
    if(state->mode == MODE_HANDLER){
        int preempt_prio = cm_NVIC_get_preempt_proi(prio, NVIC_info);
        int handling_preempt_prio = cm_NVIC_get_preempt_proi(cpu->cm_NVIC->prio_table[cur_exception], NVIC_info);
        if(preempt_prio < handling_preempt_prio){
            bheap_delete_top(NVIC_info->pending_list, &mod_prio, bheap_compare_int_smaller);
            retval = mod_prio & 0x1FFul;
        }else{
            retval = 0;
        }
    }else{
        bheap_delete_top(NVIC_info->pending_list, &mod_prio, bheap_compare_int_smaller);
        retval = mod_prio & 0x1FFul;
    }
    return retval;
}

int cm_NVIC_handle_exception(int vector_num, cpu_t* cpu)
{
    PushStack(vector_num, cpu);
    ExceptionTaken(vector_num, cpu);
    return 0;
}
