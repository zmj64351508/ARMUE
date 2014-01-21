#ifndef _CM_NVIC_H_
#define _CM_NVIC_H_
#ifdef __cplusplus
extern "C"{
#endif

#include "cpu.h"
#include "bheap.h"
#include "exception_interrupt.h"
#include <stdint.h>

#define NVIC_MAX_EXCEPTION 496

typedef bheap_t pending_list_t;

typedef struct cm_NVIC_t
{
    uint8_t exception_active[NVIC_MAX_EXCEPTION];
    uint8_t nested_exception;
    uint8_t preempt_mask;
    uint8_t prio_mask;
    uint8_t interrupt_lines;
    pending_list_t* pending_list;
}cm_NVIC_t;

void ExceptionReturn(uint32_t exc_return, cpu_t *cpu);
int ExecutionPriority(cpu_t *cpu);

vector_exception_t* cm_NVIC_init(cpu_t* cpu);
int cm_NVIC_startup(cpu_t *cpu);
int cm_NVIC_throw_exception(int vector_num, struct vector_exception_t* controller);
int cm_NVIC_check_exception(cpu_t *cpu);
int cm_NVIC_handle_exception(int vector_num, cpu_t* cpu);

#ifdef __cplusplus
}
#endif
#endif
