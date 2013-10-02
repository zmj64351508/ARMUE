#ifndef _CM_NVIC_H_
#define _CM_NVIC_H_
#ifdef __cplusplus
extern "C"{
#endif

#include "bheap.h"

typedef bheap_t pending_list_t;

typedef struct cm_NVIC_t
{
	pending_list_t* pending_list;
	uint8_t preempt_mask;
	uint8_t prio_mask;
}cm_NVIC_t;

int cm_NVIC_init(cpu_t* cpu);
int cm_NVIC_throw_exception(int vector_num, struct vector_exception_t* controller);
int cm_NVIC_check_exception(cpu_t *cpu);
int cm_NVIC_handle_exception(int vector_num, cpu_t* cpu);

#ifdef __cplusplus
}
#endif
#endif