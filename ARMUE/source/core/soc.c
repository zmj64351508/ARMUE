#include "module_helper.h"
#include "soc.h"
//#include "arm_gdb_stub.h"
#include <stdlib.h>
#include <unistd.h>
#include <windows.h>
#include "config.h"

int startup_soc(soc_t* soc)
{
    if(soc == NULL || soc->cpu == NULL || soc->cpu[0]->memory_map == NULL){
        return ERROR_NULL_POINTER;
    }
    int retval = -ERROR_SOC_STARTUP;

    cpu_t *cpu = soc->cpu[0];

    // halt the cpu at startup
    if(config.gdb_debug){
        cpu->run_info.halting = TRUE;
    }
    cpu->run_info.last_pc = 0;

    /* start the cpu */
    if(cpu->startup != NULL){
        retval = cpu->startup(cpu);
    }

    return retval;
}

#include "arm_v7m_ins_decode.h"
uint32_t run_soc(soc_t* soc)
{
    cpu_t *cpu = soc->cpu[0];

    if(config.gdb_debug){
        LOG(LOG_DEBUG, "last pc is %x\n", cpu->run_info.last_pc);
        if(cpu->run_info.halting == MAYBE){
            /* The break operation code is set by debugger. So the last operation code executed
               should be a break trap set by the debugger. That means we should re-execute the
               operation code in that address. Restore last PC value to the PC can do such thing.*/
            if(is_sw_breakpoint(soc->stub, cpu->run_info.last_pc)){
                cpu->run_info.halting = TRUE;
                cpu->set_raw_pc(cpu->run_info.last_pc, cpu);
            }
            // The break operation code is directly wrote in the program
            else{
                cpu->run_info.halting = FALSE;
            }
        }

        if(soc->stub->status == RSP_STEP){
            cpu->run_info.halting = TRUE;
        }

        /* cpu halting for debug */
        while(cpu->run_info.halting){
            handle_rsp(soc->stub, cpu);
        }
    }

    /* store last pc */
    cpu->run_info.last_pc = cpu->get_raw_pc(cpu);

    /* basic steps to run a single operation code */
    uint32_t opcode   = cpu->fetch32(cpu);
    ins_t    ins_info = cpu->decode(cpu, &opcode);
                        cpu->excute(cpu, ins_info);

    /* exception and interrupt checker/handler */
    if(cpu->GIC){

    }
    uint32_t vector_num = cpu->exceptions->check_exception(cpu);
    if(vector_num != 0){
        cpu->exceptions->handle_exception(vector_num, cpu);
    }
    add_cycle(cpu);
    LOG_REG(cpu);
    //getchar();
    return opcode;
}

static soc_t* create_single_cpu_soc(cpu_t* cpu, memory_map_t *memory_space, memory_map_t *io_space)
{
    soc_t* soc = (soc_t*)calloc(1, sizeof(soc_t));
    if(soc == NULL){
        return NULL;
    }

    /* cpu id starts from 0 */
    soc->cpu_num = 1;
    cpu->cid = 0;
    soc->cpu[0] = cpu;
    soc->cpu[0]->memory_space = memory_space;
    soc->cpu[0]->io_space = io_space;

    return soc;
}

/* create the soc and initialize the content */
soc_t* create_soc(soc_conf_t* config)
{
    soc_t* soc = NULL;
    if(config->cpu_num != 1)
        return NULL;

    /* find the cpu module and allocate the cpu */
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

    /* create the soc */
    LOG(LOG_DEBUG, "create_soc: created cpu %s\n", cpu_module->name);
    soc = create_single_cpu_soc(cpu, config->memories[0], config->memories[0]);
    if(soc == NULL){
        goto create_soc_fail;
    }

    /* Initialize the cpu. It is the cpu specific action.
       CPU need to know the memory map and exceptions, so before init_cpu,
       it must ensure exceptions and memory map are setuped. */
    int retval = cpu_module->init_cpu(cpu, config);
    if(retval < 0){
        goto init_cpu_fail;
    }

    /* validate the cpu */
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
    for(i = 0; i < (*soc)->cpu_num; i++){
        module_t* cpu_module = (module_t*)get_cpu_module((*soc)->cpu[i]);
        if(cpu_module != NULL){
            cpu_module->destory_cpu(&(*soc)->cpu[i]);
        }
    }

    free(*soc);
    *soc = NULL;

    return SUCCESS;
}
