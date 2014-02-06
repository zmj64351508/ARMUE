#ifndef _SOC_H_
#define _SOC_H_
#ifdef __cplusplus
extern "C"{
#endif

#include "cpu.h"
#include "memory_map.h"
#include "error_code.h"
//#include "soc.h"
#include "arm_gdb_stub.h"

#define MAX_CPU_NUM 2
#define MEMORY_NUM_MAX 2

typedef struct {
    int cpu_num;
    gdb_stub_t *stub;
    cpu_t *cpu[MAX_CPU_NUM];
    void *global_info;
    list_t *timer_list;
}soc_t;

typedef struct soc_conf_t{
    int cpu_num;
    char* cpu_name;

    int exception_num;
    int nested_level;

    int has_GIC;
    int GIC_interrupt_num;
    int GIC_nested_level;

    int memory_map_num;
    memory_map_t* memories[MEMORY_NUM_MAX];

    /* thumb specific configuration */
    uint32_t exclusive_high_address;   // exclusive addr
    uint32_t exclusive_low_address;
}soc_conf_t;

soc_t* create_soc(soc_conf_t* config);
error_code_t destory_soc(soc_t **soc);
int startup_soc(soc_t* soc);
uint32_t run_soc(soc_t* soc);

#ifdef __cplusplus
}
#endif
#endif
