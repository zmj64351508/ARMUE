#include "cpu.h"
#include "arm_v7m_ins_implement.h"
#include "cm_NVIC.h"
#include <stdlib.h>

enum cm_NVIC_prio{
	CM_NVIC_PRIO_RESET		=	-3,
	CM_NVIC_PRIO_NMI		=	-2,
	CM_NVIC_PRIO_HARDFAULT	=	-1,
	CM_NVIC_PRIO_UNKNOW		=	 0,
};

enum cm_NVIC_vector{
	CM_NVIC_VEC_RESET		=	1,
	CM_NVIC_VEC_NMI			=	2,
	CM_NVIC_VEC_HARDFAULT	=	3,
	CM_NVIC_VEC_MEMMANFAULT	=	4,
	CM_NVIC_VEC_BUSFAULT	=	5,
	CM_NVIC_VEC_USAGEFAULT	=	6,
	CM_NVIC_VEC_SVCALL		=	11,
	CM_NVIC_VEC_DEBUG		=	12,
	CM_NVIC_VEC_PENDSV		=	14,
	CM_NVIC_VEC_SYSTICK		=	15,
};

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

void cm_NVIC_vector_table_init(vector_exception_t *controller, memory_map_t *memory)
{
	uint32_t addr = 0;
	uint32_t interrput_vector = 0;
	int inpterrupt_table_size = controller->vector_table_size;
	for(int i = 0; i < inpterrupt_table_size; i++){
		read_memory(addr, (uint8_t*)&interrput_vector, 4, memory);
		set_vector_table(controller, interrput_vector, i);
		addr += 4;
	}
}

void cm_NVIC_prio_table_init(vector_exception_t *controller)
{
	int i;
	for(i = 0; i < controller->vector_table_size; i++){
		controller->prio_table[i] = CM_NVIC_PRIO_UNKNOW;
	}
	controller->prio_table[CM_NVIC_VEC_RESET]		= CM_NVIC_PRIO_RESET;
	controller->prio_table[CM_NVIC_VEC_NMI]			= CM_NVIC_PRIO_NMI;
	controller->prio_table[CM_NVIC_VEC_HARDFAULT]	= CM_NVIC_PRIO_HARDFAULT;
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

int setup_cm_NVIC_info(vector_exception_t* controller)
{
	cm_NVIC_t* info = (cm_NVIC_t*)controller->controller_info;

	info->pending_list = bheap_create(controller->vector_table_size, sizeof(uint32_t));
	if(info->pending_list == NULL){
		goto pending_list_null;
	}

	info->preempt_mask = 0xF;
	info->prio_mask = 0xF;
	return 0;

pending_list_null:
	return -1;
}

int cm_NVIC_init(cpu_t* cpu)
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

	cpu->cm_NVIC->controller_info = create_cm_NVIC_info();
	if(cpu->cm_NVIC->controller_info == NULL){
		return -ERROR_CREATE;
	}
	if(setup_cm_NVIC_info(cpu->cm_NVIC) < 0){
		return -ERROR_CREATE;
	}

	cpu->cm_NVIC->throw_exception = cm_NVIC_throw_exception;
	cpu->cm_NVIC->check_exception = cm_NVIC_check_exception;
	cpu->cm_NVIC->handle_exception = cm_NVIC_handle_exception;

	cm_NVIC_vector_table_init(cpu->cm_NVIC, memory_map);
	cm_NVIC_prio_table_init(cpu->cm_NVIC);

	return SUCCESS;
}

int cm_NVIC_throw_exception(int vector_num, struct vector_exception_t* controller)
{
	cm_NVIC_t* NVIC_info = (cm_NVIC_t*)controller->controller_info;
	int prio = controller->prio_table[vector_num];
	int mod_prio = (prio << 8) | (vector_num & 0xFFul);
	return bheap_insert(NVIC_info->pending_list, &mod_prio, bheap_compare_int_smaller);
}

int cm_NVIC_check_exception(cpu_t* cpu)
{
	cm_NVIC_t* NVIC_info = (cm_NVIC_t*)cpu->cm_NVIC->controller_info;
	int mod_prio;
	int retval = bheap_peek_top(NVIC_info->pending_list, &mod_prio);
	if(retval < 0){
		return 0;
	}

	int prio = mod_prio >> 8;
	thumb_state* state = ARMv7m_GET_STATE(cpu);
	/* If in handler, check preempt priority for the preemption */
	if(state->mode == MODE_HANDLER){
		int preempt_prio = cm_NVIC_get_preempt_proi(prio, NVIC_info);
		int handling_preempt_prio = cm_NVIC_get_preempt_proi(cpu->cm_NVIC->prio_table[state->cur_exception], NVIC_info); 
		if(preempt_prio < handling_preempt_prio){
			bheap_delete_top(NVIC_info->pending_list, &mod_prio, bheap_compare_int_smaller);
			retval = mod_prio & 0xFFul;
		}else{
			retval = 0;
		}
	}else{
		bheap_delete_top(NVIC_info->pending_list, &mod_prio, bheap_compare_int_smaller);
		retval = mod_prio & 0xFFul;
	}
	return retval;
}

int cm_NVIC_handle_exception(int vector_num, cpu_t* cpu)
{
	uint32_t addr = get_vector_value(cpu->cm_NVIC, vector_num);
	cpu->run_info.next_ins = 0;
	/* TODO: Push xPSR PC LR R12 R3-R0 */

	thumb_state* state = ARMv7m_GET_STATE(cpu);
	armv7m_reg_t *regs = (armv7m_reg_t*)cpu->regs;

	/* if defined double word align */
	uint32_t SP_val = GET_REG_VAL(regs, SP_INDEX);
	SP_val = DOWN_ALIGN(SP_val, 3);
	SET_REG_VAL(regs, SP_INDEX, SP_val);

	uint32_t PSR_val = GET_PSR(regs);
	armv7m_push_reg(PSR_val, cpu);
	armv7m_push_reg(GET_REG_VAL(regs, PC_INDEX), cpu);
	armv7m_push_reg(GET_REG_VAL(regs, LR_INDEX), cpu);
	armv7m_push_reg(GET_REG_VAL(regs, 12), cpu);
	armv7m_push_reg(GET_REG_VAL(regs, 3), cpu);
	armv7m_push_reg(GET_REG_VAL(regs, 2), cpu);
	armv7m_push_reg(GET_REG_VAL(regs, 1), cpu);
	armv7m_push_reg(GET_REG_VAL(regs, 0), cpu);

	uint32_t new_LR = 0xFFFFFFF9 | ((regs->CONTROL << 1) & 0x4);
	if(state->mode == MODE_HANDLER){
		new_LR &= ~(1ul << 3);
	}
	SET_REG_VAL(regs, LR_INDEX, new_LR);

	SET_PSR(regs, (PSR_val & 0xFF) | vector_num);
	armv7m_branch(addr, cpu);
	
	state->mode = MODE_HANDLER;
	state->cur_exception = vector_num;
	return 0;
}