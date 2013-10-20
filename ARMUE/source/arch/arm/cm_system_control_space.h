#ifndef _CM_SYSTEM_CONTROL_SPACE_H_
#define _CM_SYSTEM_CONTROL_SPACE_H_
#ifdef __cplusplus
extern "C"{
#endif

#include "cm_NVIC.h"
#include "memory_map.h"

#define CM_SCS_BASE 0xE000E000
#define CM_SCS_SIZE 4096

typedef struct cm_scs_t{
    void* user_defined_data;
    vector_exception_t *NVIC;
    //cm_systick_t *systick;
    int (*user_defined_read)(uint32_t offset, uint8_t *buffer, int size, struct cm_scs_t *scs);
    int (*user_defined_write)(uint32_t offset, uint8_t *buffer, int size, struct cm_scs_t *scs);
}cm_scs_t;

int cm_scs_init(cpu_t *cpu);

#ifdef __cplusplus
}
#endif
#endif