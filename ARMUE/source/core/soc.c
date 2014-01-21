#include "module_helper.h"
#include "soc.h"
#include <stdlib.h>


int startup_soc(soc_t* soc)
{
    if(soc == NULL || soc->cpu == NULL || soc->cpu[0]->memory_map == NULL){
        return ERROR_NULL_POINTER;
    }
    int retval = -ERROR_SOC_STARTUP;

    cpu_t *cpu = soc->cpu[0];

    if(cpu->startup != NULL){
        retval = cpu->startup(cpu);
    }

    return retval;
}

#include "arm_v7m_ins_decode.h"
uint32_t run_soc(soc_t* soc)
{
    cpu_t *cpu = soc->cpu[0];
    uint32_t opcode = cpu->fetch32(cpu);
    ins_t ins_info = cpu->decode(cpu, &opcode);
    cpu->excute(cpu, ins_info);
    if(cpu->GIC){

    }
    uint32_t vector_num = cpu->exceptions->check_exception(cpu);
    if(vector_num != 0){
        cpu->exceptions->handle_exception(vector_num, cpu);
    }
    add_cycle(cpu);
    LOG_REG(cpu);
    getchar();
    return opcode;
}

static soc_t* create_single_cpu_soc(cpu_t* cpu, memory_map_t *memory_space, memory_map_t *io_space)
{
    soc_t* soc = (soc_t*)calloc(1, sizeof(soc_t));
    if(soc == NULL){
        return NULL;
    }

    soc->cpu[0] = cpu;
    soc->cpu[0]->memory_space = memory_space;
    soc->cpu[0]->io_space = io_space;

    return soc;
}

soc_t* create_soc(soc_conf_t* config)
{
    soc_t* soc = NULL;
    if(config->cpu_num != 1)
        return NULL;

    module_t* cpu_module = find_module(config->cpu_name);
    if(cpu_module == NULL){
        goto find_cpu_module_fail;
    }

    cpu_t* cpu = alloc_cpu();
    if(cpu == NULL){
        goto alloc_cpu_fail;
    }

    /* create internal exception controller and external GIC */
    cpu->exceptions = create_vector_exception(config->exception_num);
    if(cpu->exceptions == NULL){
        goto exception_null;
    }
    if(config->has_GIC){
        cpu->GIC = create_vector_exception(config->GIC_interrupt_num);
        if(cpu->GIC == NULL){
            goto GIC_null;
        }
    }else{
        cpu->GIC = NULL;
    }

    LOG(LOG_DEBUG, "create_soc: created cpu %s\n", cpu_module->name);
    soc = create_single_cpu_soc(cpu, config->memories[0], config->memories[0]);
    if(soc == NULL){
        goto create_soc_fail;
    }

    /* Before init_cpu, it must ensure exceptions and memory map are setuped. */
    int retval = cpu_module->init_cpu(cpu, config);
    if(retval < 0){
        goto init_cpu_fail;
    }
    if(!validate_cpu(cpu)){
        LOG(LOG_ERROR, "create_soc: invalid cpu %s\n", cpu_module->name);
        goto invalid_cpu;
    }

    /* global info is created when the first core of the cpu is initialized */
    if(cpu->run_info.global_info != NULL){
        soc->global_info = cpu->run_info.global_info;
    }
    return soc;

invalid_cpu:
    cpu_module->destory_cpu(&cpu);
init_cpu_fail:
    destory_soc(&soc);
create_soc_fail:
    if(cpu->GIC){
        destory_vector_exception(&cpu->GIC);
    }
GIC_null:
    destory_vector_exception(&cpu->exceptions);
exception_null:
    dealloc_cpu(&cpu);
alloc_cpu_fail:
find_cpu_module_fail:
    return NULL;
}

error_code_t destory_soc(soc_t **soc)
{
    if(soc == NULL || *soc == NULL){
        return ERROR_NULL_POINTER;
    }

    int i;
    for(i = 0; i < SOC_CPU_MAX; i++){
        module_t* cpu_module = (module_t*)get_cpu_module((*soc)->cpu[i]);
        if(cpu_module != NULL){
            cpu_module->destory_cpu(&(*soc)->cpu[i]);
        }
    }

    free(*soc);
    *soc = NULL;

    return SUCCESS;
}
