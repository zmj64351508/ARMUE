#include "module_helper.h"
#include "soc.h"
#include <stdlib.h>


error_code_t startup_soc(soc_t* soc)
{
	if(soc == NULL || soc->cpu == NULL || soc->cpu[0]->memory_map == NULL){
		return ERROR_NULL_POINTER;
	}
	error_code_t retval = ERROR_SOC_STARTUP;

	if(soc->cpu[0]->startup != NULL){	
		retval = soc->cpu[0]->startup(soc->cpu[0]);
	}

	return retval;
}

uint32_t run_soc(soc_t* soc)
{
	uint32_t instruction = soc->cpu[0]->fetch32(soc->cpu[0]);
	soc->cpu[0]->excute(soc->cpu[0], &instruction);
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