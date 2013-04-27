#ifndef _SOC_H_
#define _SOC_H_

#include "cpu.h"
#include "memory_map.h"
#include "error_code.h"

#define SOC_CPU_MAX 2

typedef struct {
	cpu_t* cpu[SOC_CPU_MAX];
}soc_t;

soc_t* create_soc(cpu_t* cpu, memory_map_t* memory_map);
error_code_t destory_soc(soc_t **soc);
error_code_t startup_soc(soc_t* soc);
uint32_t run_soc(soc_t* soc);

#endif