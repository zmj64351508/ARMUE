#include "module_helper.h"
#include "soc.h"
#include <stdlib.h>


error_code_t startup_soc(soc_t* soc)
{	
	if(soc == NULL || soc->cpu == NULL || soc->cpu[0]->memory_map == NULL){
		return ERROR_NULL_POINTER;
	}
	error_code_t retval = ERROR_SOC_STARTUP;

	cpu_t *cpu = soc->cpu[0];

	if(cpu->startup != NULL){	
		retval = cpu->startup(cpu);
	}

	return retval;
}

uint32_t run_soc(soc_t* soc)
{
	cpu_t *cpu = soc->cpu[0];
	uint32_t instruction = cpu->fetch32(cpu);
	cpu->excute(cpu, &instruction);
	return instruction;
}

soc_t* create_soc(cpu_t* cpu, memory_map_t* memory_map)
{
	soc_t* soc = (soc_t*)calloc(1, sizeof(soc_t));

	soc->cpu[0] = cpu;
	soc->cpu[0]->memory_map = memory_map;

	return soc;
}

error_code_t destory_soc(soc_t **soc)
{
	if(soc == NULL || *soc == NULL){
		return ERROR_NULL_POINTER;
	}

	for(int i = 0; i < SOC_CPU_MAX; i++){
		module_t* cpu_module = (module_t*)get_cpu_module((*soc)->cpu[i]);
		if(cpu_module != NULL){
			cpu_module->destory_cpu(&(*soc)->cpu[i]);
		}
	}

	free(*soc);
	*soc = NULL;

	return SUCCESS;
}