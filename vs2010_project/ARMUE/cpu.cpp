#include "cpu.h"
#include <stdlib.h>

cpu_list_t* create_cpu_list()
{
	//LOG(LOG_DEBUG, "create_cpu_list\n");
	cpu_list_t* cpu_list = (cpu_list_t*)calloc(1, sizeof(cpu_list_t));
	return cpu_list;
}

cpu_t* alloc_cpu()
{
	//LOG(LOG_DEBUG, "create_cpu_helper\n");
	cpu_t* cpu = (cpu_t*)calloc(1, sizeof(cpu_t));
	return cpu;
}

error_code_t set_cpu_type(cpu_t* cpu, cpu_type_t type)
{
	if(cpu == NULL){
		return ERROR_NULL_POINTER;
	}

	cpu->type = type;

	return SUCCESS;
}

error_code_t set_cpu_spec_info(cpu_t* cpu, void* info)
{
	if(cpu == NULL){
		return ERROR_NULL_POINTER;
	}

	cpu->run_info.cpu_spec_info = info;
	return SUCCESS;
}

error_code_t set_cpu_startup_func(cpu_t* cpu, cpu_startup_func_t startup_func)
{
	if(cpu == NULL){
		return ERROR_NULL_POINTER;
	}

	cpu->startup = startup_func;
	return SUCCESS;
}

error_code_t set_cpu_exec_func(cpu_t* cpu, cpu_exec_func_t run_func)
{
	if(cpu == NULL){
		return ERROR_NULL_POINTER;
	}

	cpu->excute = run_func;
	return SUCCESS;
}

error_code_t set_cpu_fetch32_func(cpu_t* cpu, cpu_fetch32_func_t fetch_func)
{
	if(cpu == NULL){
		return ERROR_NULL_POINTER;
	}

	cpu->fetch32 = fetch_func;
	return SUCCESS;
}

error_code_t set_cpu_module(cpu_t* cpu, void* module)
{
	if(cpu == NULL || module == NULL){
		return ERROR_NULL_POINTER;
	}

	cpu->module = module;
	return SUCCESS;
}

void* get_cpu_module(cpu_t* cpu)
{
	if(cpu == NULL){
		return NULL;
	}

	return cpu->module;
}


cpu_t* get_first_cpu(cpu_list_t* cpu_list)
{
	return cpu_list->first_cpu;
}

cpu_t* get_next_cpu(cpu_t *cpu)
{
	return cpu->next_cpu;
}

error_code_t add_cpu_to_tail(cpu_list_t* list, cpu_t* cpu)
{
	cpu_t* last_cpu = list->last_cpu;
	cpu_t* first_cpu = list->first_cpu;

	if(list->last_cpu == NULL){
		list->first_cpu = cpu;
		list->last_cpu = cpu;
	}else{
		last_cpu->next_cpu = cpu;
		cpu->pre_cpu = last_cpu;
		list->last_cpu = cpu;
	}

	list->count++;
	cpu->list = list;

	return SUCCESS;
}

error_code_t delete_cpu_from_head(cpu_list_t* list, cpu_t* cpu)
{
	if(list == NULL || cpu == NULL){
		return ERROR_NULL_POINTER;
	}

	if(cpu->list != list){
		return ERROR_DISMATCH_LIST;
	}

	if(list->count == 1){
		list->first_cpu = NULL;
		list->last_cpu = NULL;
	}else{
		list->first_cpu = cpu->next_cpu;
		list->first_cpu->pre_cpu = NULL;
	}

	list->count--;

	return SUCCESS;
}

error_code_t delete_cpu_from_tail(cpu_list_t* list, cpu_t* cpu)
{
	if(list == NULL || cpu == NULL){
		return ERROR_NULL_POINTER;
	}

	if(cpu->list != list){
		return ERROR_DISMATCH_LIST;
	}


	if(list->count == 1){
		list->first_cpu = NULL;
		list->last_cpu = NULL;
	}else{
		list->last_cpu = cpu->pre_cpu;
		list->last_cpu->next_cpu = NULL;
	}

	list->count--;

	return SUCCESS;
}

error_code_t delete_cpu_from_middle(cpu_list_t* list, cpu_t* cpu)
{
	if(list == NULL || cpu == NULL){
		return ERROR_NULL_POINTER;
	}

	if(cpu->list != list){
		return ERROR_DISMATCH_LIST;
	}

	cpu_t* pre_cpu = cpu->pre_cpu;
	cpu_t* next_cpu = cpu->next_cpu;

	if(list->count == 1){
		list->first_cpu = NULL;
		list->last_cpu = NULL;
	}else{
		pre_cpu->next_cpu = next_cpu;
		next_cpu->pre_cpu = pre_cpu;
	}

	list->count--;

	return SUCCESS;

}

error_code_t delete_cpu(cpu_list_t* list, cpu_t* cpu)
{
	if(list == NULL || cpu == NULL){
		return ERROR_NULL_POINTER;
	}

	error_code_t error_code = SUCCESS;

	if(cpu->pre_cpu == NULL){
		error_code = delete_cpu_from_head(list, cpu);
	}else if(cpu->next_cpu == NULL){
		error_code = delete_cpu_from_tail(list, cpu);
	}else{
		error_code = delete_cpu_from_middle(list, cpu);
	}

	return error_code;
}

error_code_t dealloc_cpu(cpu_t** cpu)
{
	if(cpu == NULL || *cpu == NULL)
		return ERROR_NULL_POINTER;

	destory_memory_map(&(*cpu)->memory_map);
	free(*cpu);
	*cpu = NULL;

	return SUCCESS;
}

error_code_t destory_cpu_list(cpu_list_t** cpu_list)
{	
	//LOG(LOG_DEBUG, "destory_cpu_list\n");
	if(*cpu_list == NULL || cpu_list == NULL){
		LOG(LOG_ERROR, "destory_cpu_list: invalid parameters\n");
		return ERROR_NULL_POINTER;
	}

		
	cpu_t* cpu, *cpu_next = NULL;
	cpu = get_first_cpu(*cpu_list);

	if(cpu != NULL){
		do{
			cpu_next = get_next_cpu(cpu);
			dealloc_cpu(&cpu);
			cpu = cpu_next;
		}while(cpu != NULL);
	}

	free(*cpu_list);
	*cpu_list = NULL;

	return SUCCESS;
}

int validate_cpu(cpu_t* cpu)
{
	if(cpu == NULL){
		return 0;
	}
	if(cpu->startup == NULL || cpu->decode == NULL){
		return 0;
	}
	if(cpu->fetch32 == NULL){
		return 0;
	}
	return 1;
}