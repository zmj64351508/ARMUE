#include "cpu.h"
#include "arm_v7m_ins_implement.h"
#include "cm_NVIC.h"

typedef struct cm_NVIC_t
{
	uint8_t prio_group_mask;
}cm_NVIC_t;

int cm_NVIC_throw_exception(int vector_num, struct vector_exception_t* controller)
{
	fifo_in(controller->current_fifo, &vector_num);
	return 0;
}

int cm_NVIC_check_exception(cpu_t *cpu)
{
	if(fifo_empty(cpu->cm_NVIC->current_fifo)){
		return 0;
	}
	uint32_t vector_num;
	fifo_out(cpu->cm_NVIC->current_fifo, &vector_num);
	return vector_num;
}

int cm_NVIC_handle_exception(int vector_num, cpu_t* cpu)
{
	uint32_t addr = get_vector_value(cpu->cm_NVIC, vector_num);
	cpu->run_info.next_ins = 0;
	/* TODO: Push xPSR PC LR R12 R3-R0 */

	/* TODO: Update PC LR SP */

	armv7m_branch(addr, cpu);
	
	thumb_state* state = ARMv7m_GET_STATE(cpu);
	state->mode = MODE_HANDLER;
	return 0;
}