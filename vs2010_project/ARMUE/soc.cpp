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
	uint32_t opcode = cpu->fetch32(cpu);
	ins_t ins_info = cpu->decode(cpu, &opcode);
	cpu->excute(cpu, ins_info);
	return opcode;
}

static soc_t* create_single_mem_soc(cpu_t* cpu, memory_map_t* memory_map)
{
	soc_t* soc = (soc_t*)calloc(1, sizeof(soc_t));

	soc->cpu[0] = cpu;
	soc->cpu[0]->memory_space = memory_map;
	soc->cpu[0]->io_space = memory_map;

	return soc;
}

soc_t* create_soc(soc_conf_t* config)
{
	soc_t* soc = NULL;
	if(config->cpu_num != 1)
		goto err_out;

	module_t* cpu_module = find_module(config->cpu_name);
	cpu_t* cpu = cpu_module->create_cpu();
	if(!validate_cpu(cpu)){
		LOG(LOG_ERROR, "create_soc: invalid cpu %s\n", cpu_module->name);
		goto invalid_cpu;
	}
	LOG(LOG_DEBUG, "create_soc: created cpu %s\n", cpu_module->name);
	if(config->memory_map_num == 1){
		soc = create_single_mem_soc(cpu, config->memories[0]);
	}else{

	}
	return soc;

invalid_cpu:
	cpu_module->destory_cpu(&cpu);
err_out:
	return NULL;
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