#ifndef _SOC_H_
#define _SOC_H_

#include "cpu.h"
#include "memory_map.h"
#include "error_code.h"

#define SOC_CPU_MAX 2
#define MEMORY_NUM_MAX 2

typedef struct {
	cpu_t* cpu[SOC_CPU_MAX];
}soc_t;

typedef struct soc_conf_t{
	int cpu_num;
	char* cpu_name;
	
	int memory_map_num;
	memory_map_t* memories[MEMORY_NUM_MAX];
}soc_conf_t;

soc_t* create_soc(soc_conf_t* config);
error_code_t destory_soc(soc_t **soc);
error_code_t startup_soc(soc_t* soc);
uint32_t run_soc(soc_t* soc);

#endif