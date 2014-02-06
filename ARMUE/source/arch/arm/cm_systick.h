#ifndef _CM_SYSTICK_H_
#define _CM_SYSTICK_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>

struct systick_reg_t{
    uint32_t SYST_CSR;
    uint32_t SYST_RVR;
    uint32_t SYST_CVR;
    uint32_t SYST_CALIB;
};
typedef struct systick_reg_t systick_reg_t;

struct cm_scs_t;
int SYST_CSR(uint8_t* data, int rw_flag, struct cm_scs_t *scs);
int SYST_RVR(uint8_t *data, int rw_flag, struct cm_scs_t *scs);
int SYST_CVR(uint8_t *data, int rw_flag, struct cm_scs_t *scs);
int SYST_CALIB(uint8_t *data, int rw_flag, struct cm_scs_t *scs);

#include "cm_system_control_space.h"

#ifdef __cplusplus
}
#endif

#endif // _CM_SYSTICK_H_
