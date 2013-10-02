#ifndef _EXCEPTION_INTERRUPT_H_
#define _EXCEPTION_INTERRUPT_H_
#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>
#include "fifo.h"

struct cpu_t;
typedef struct vector_exception_t
{
	uint32_t *vector_table;
	int *prio_table;
	int vector_table_size;
	void *controller_info;

	int (*throw_exception)(int vector_num, struct vector_exception_t* controller);
	int (*check_exception)(struct cpu_t* cpu);
	int (*handle_exception)(int vector_num, struct cpu_t* cpu);
}vector_exception_t;
typedef vector_exception_t vector_interrupt_t;

vector_exception_t* create_vector_exception(int table_size);
void destory_vector_exception(vector_exception_t** exceptions);
int set_vector_table(vector_exception_t* controller, uint32_t intrpt_value, int intrpt_num);
uint32_t get_vector_value(vector_exception_t* controller, int exception_num);


#ifdef __cplusplus
}
#endif
#endif