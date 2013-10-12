#ifndef _CPU_H_
#define _CPU_H_
#ifdef __cplusplus
extern "C"{
#endif

#include "error_code.h"
#include "memory_map.h"
#include "exception_interrupt.h"

typedef enum{
	CPU_INVALID,
	CPU_ARM_CM3,
}cpu_type_t;

typedef struct ins_t{
	unsigned long long opcode;
	void *excute;
	int length;
}ins_t;

struct cpu_t;
typedef uint32_t (*cpu_fetch32_func_t)(struct cpu_t* cpu);
typedef ins_t (*cpu_decode_func_t)(struct cpu_t* cpu, void* opcode);
typedef void (*cpu_exec_func_t)(struct cpu_t* cpu, ins_t opcode);
typedef int (*cpu_startup_func_t)(struct cpu_t* cpu);

typedef struct cpu_list_t
{
	int count;
	struct cpu_t* first_cpu;
	struct cpu_t* last_cpu;
}cpu_list_t;

typedef struct run_info_t
{
	unsigned long long last_pc;
	unsigned long long next_ins;
	void *cpu_spec_info;
	void *global_info;
	int ins_type;
}run_info_t;

typedef struct cpu_t
{
	cpu_type_t type;	// cpu type
	int cid;			// for multi-cpu soc, not use yet

	run_info_t run_info;
	void *regs;
	void *system_info;
	void *instruction_data;

	/* For cortex-m profile, NVIC is internal with function of exception controller and
	   general interrupt controller, while other profile like A, R and classical ARM cpu
	   has an internal exception controller and an external interrupt controller. */
	union{
		vector_exception_t* exceptions;
		vector_interrupt_t* cm_NVIC;
	};
	vector_interrupt_t* GIC;

	/* for some architecture with seperated io and memory space.
	   If the architecture has only single io and memory space,
	   these 2 pointer should pointer to the same memory map*/
	union{
		memory_map_t* memory_space;
		memory_map_t* memory_map;
	};
	memory_map_t* io_space;

	void* module;		// which cpu module it belongs to

	/* interfaces */
	cpu_startup_func_t startup;
	cpu_fetch32_func_t fetch32;
	cpu_decode_func_t decode;
	cpu_exec_func_t excute;

	// cpu list
	struct cpu_t* next_cpu;
	struct cpu_t* pre_cpu;
	cpu_list_t* list;
}cpu_t;

cpu_list_t*		create_cpu_list();
error_code_t	destory_cpu_list(cpu_list_t** cpu_list);

cpu_t*			alloc_cpu();
error_code_t	dealloc_cpu(cpu_t** cpu);

error_code_t add_cpu_to_tail(cpu_list_t* list, cpu_t* cpu);
error_code_t delete_cpu(cpu_list_t* list, cpu_t* cpu);

error_code_t set_cpu_type(cpu_t* cpu, cpu_type_t type);
error_code_t set_cpu_spec_info(cpu_t* cpu, void* info);
error_code_t set_cpu_module(cpu_t* cpu, void* module);

void*	get_cpu_module(cpu_t* cpu);
int validate_cpu(cpu_t* cpu);

#ifdef __cplusplus
}
#endif
#endif
