#ifndef _CM_SYSTEM_CONTROL_SPACE_H_
#define _CM_SYSTEM_CONTROL_SPACE_H_
#ifdef __cplusplus
extern "C"{
#endif

#include "cm_systick.h"
#include "cm_NVIC.h"
#include "memory_map.h"
#include "cpu.h"
#include "timer.h"

#define CM_SCS_BASE 0xE000E000
#define CM_SCS_SIZE 4096


#define LITTLE_ENDIAN   0
#define BIG_ENDIAN      1

typedef struct cm_config_t{
    int endianess;
    int prigroup;
}cm_config_t;

typedef struct cm_scs_reg_t{
    uint32_t DHCSR;
}cm_scs_reg_t;

struct cm_scs_t{
    cm_config_t config;
    cm_scs_reg_t regs;
    struct systick_reg_t systick_regs;
    timer_t *systick;
    cpu_t *cpu;
    void* user_defined_data;
    vector_exception_t *NVIC;
    //cm_systick_t *systick;
    int (*user_defined_read)(uint32_t offset, uint8_t *buffer, int size, struct cm_scs_t *scs);
    int (*user_defined_write)(uint32_t offset, uint8_t *buffer, int size, struct cm_scs_t *scs);
};
typedef struct cm_scs_t cm_scs_t;

int cm_scs_init(cpu_t *cpu);

#ifdef __cplusplus
}
#endif
#endif
