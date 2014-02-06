#include "timer.h"
#include "cm_NVIC.h"
#include "cm_systick.h"
#include "arm_v7m_ins_implement.h"

#define CSR_COUNTFLAG (1ul << 16)
#define CSR_ENABLE (1ul)
#define SYST_REGS(scs) (scs->systick_regs)


void systick_do_match(cpu_t *cpu)
{
    LOG(LOG_DEBUG, "systick_do_match\n");
    cpu->exceptions->throw_exception(CM_NVIC_VEC_SYSTICK, cpu->exceptions);
}

int SYST_CSR(uint8_t* data, int rw_flag, cm_scs_t *scs)
{
    int retval;
    if(rw_flag == MEM_READ){
        *(uint32_t *)data = SYST_REGS(scs).SYST_CSR & 0x0001000F;
        // clear COUNTFFLAG on read
        SYST_REGS(scs).SYST_CSR &= ~CSR_COUNTFLAG;
    }else{
        // COUNTFLAG is RO
        SYST_REGS(scs).SYST_CSR &= CSR_COUNTFLAG;
        SYST_REGS(scs).SYST_CSR |= *(uint32_t *)data & (~CSR_COUNTFLAG);
        LOG(LOG_DEBUG, "writed SYST_CSR: %x\n", SYST_REGS(scs).SYST_CSR);
        cpu_t *cpu = scs->cpu;
        // create/delete timer if necessary
        if(SYST_REGS(scs).SYST_CSR & CSR_ENABLE){
            timer_t *timer = create_timer(CM_NVIC_VEC_SYSTICK);
            timer->reload = LOW_BIT32(SYST_REGS(scs).SYST_RVR, 24);
            timer->match = calc_timer_match(cpu, timer->reload);
            timer->do_match = systick_do_match;
            // store timer pointer in scs so we can access the timer without searching in the list every time
            scs->systick = timer;
            retval = add_timer(timer, cpu->timer_list, TRUE);
            return retval;
        }else{
            retval = delete_timer(CM_NVIC_VEC_SYSTICK, cpu->timer_list);
            return retval;
        }
    }
    return 0;
}

int SYST_RVR(uint8_t *data, int rw_flag, cm_scs_t *scs)
{
    if(rw_flag == MEM_READ){
        *(uint32_t *)data = LOW_BIT32(SYST_REGS(scs).SYST_RVR, 24);
    }else{
        SYST_REGS(scs).SYST_RVR = LOW_BIT32(*(uint32_t *)data, 24);
        // update reload in timer
        if(scs->systick != NULL){
            scs->systick->reload = SYST_REGS(scs).SYST_RVR;
        }
        LOG(LOG_DEBUG, "writed SYST_RVR: %x\n", SYST_REGS(scs).SYST_RVR);
    }
    return 0;
}

int SYST_CVR(uint8_t *data, int rw_flag, cm_scs_t *scs)
{
    if(rw_flag == MEM_READ){
        // calculate CVR by match and current cycle
        if(scs->systick != NULL){
            SYST_REGS(scs).SYST_CVR = scs->systick->match - scs->cpu->cycle;
        }
        *(uint32_t *)data = SYST_REGS(scs).SYST_CVR;
    }else{
        SYST_REGS(scs).SYST_CVR = *(uint32_t *)data;
        // COUNTFLAG cleared on write to this register
        SYST_REGS(scs).SYST_CSR &= ~CSR_COUNTFLAG;
        // update match by CVR
        if(scs->systick != NULL){
            scs->systick->match -= SYST_REGS(scs).SYST_CVR;
        }
        LOG(LOG_DEBUG, "writed SYST_CVR: %x\n", SYST_REGS(scs).SYST_CVR);
    }
    return 0;
}

int SYST_CALIB(uint8_t *data, int rw_flag, cm_scs_t *scs)
{
    if(rw_flag == MEM_READ){
        *(uint32_t *)data = SYST_REGS(scs).SYST_CALIB;
        return 0;
    }else{
        return -1;
    }
}
