#include "timer.h"
#include "cm_NVIC.h"
#include "cm_systick.h"
#include "arm_v7m_ins_implement.h"

#define CSR_COUNTFLAG (1ul << 16)
#define CSR_TICKINT (1ul << 1)
#define CSR_ENABLE (1ul)

#define SYST_REGS(scs) (scs->systick_regs)

#define SET_BITS(raw, bit_mask) ((raw) |= (bit_mask))
#define CLR_BITS(raw, bit_mask) ((raw) &= ~(bit_mask))
#define BITS_ARE_SET(val, bit_mask) ((val) & (bit_mask))
#define SET_IGNORE_BITS(raw, set_val, ignore_mask) ((raw) = ((raw) & (ignore_mask)) | ((set_val) & ~(ignore_mask)))

void disable_systick(cpu_t *cpu)
{
    cm_scs_t *scs = (cm_scs_t *)cpu->system_info;
    delete_timer(CM_NVIC_VEC_SYSTICK, cpu->timer_list);
    scs->systick = NULL;
}

int systick_do_match(timer_t *timer, cpu_t *cpu)
{
    LOG(LOG_DEBUG, "systick_do_match\n");
    cm_scs_t *scs = (cm_scs_t *)cpu->system_info;
    bool_t disable_flag = (bool_t)timer->user_data_int;

    // COUNTFLAG set to 1
    SET_BITS(SYST_REGS(scs).SYST_CSR, CSR_COUNTFLAG);
    // make exception pending when TICKINT bit is 1
    if(BITS_ARE_SET(SYST_REGS(scs).SYST_CSR, CSR_TICKINT)){
        cpu->exceptions->throw_exception(CM_NVIC_VEC_SYSTICK, cpu->exceptions);
    }

    if(disable_flag){
        disable_systick(cpu);
    }
    return 0;
}

int SYST_CSR(uint8_t* data, int rw_flag, cm_scs_t *scs)
{
    int retval;
    if(rw_flag == MEM_READ){
        *(uint32_t *)data = SYST_REGS(scs).SYST_CSR & 0x0001000F;

        // clear COUNTFFLAG on read
        CLR_BITS(SYST_REGS(scs).SYST_CSR, CSR_COUNTFLAG);
    }else{

        // COUNTFLAG is RO
        SET_IGNORE_BITS(SYST_REGS(scs).SYST_CSR, *(uint32_t *)data, CSR_COUNTFLAG);
        LOG(LOG_DEBUG, "writed SYST_CSR: %x\n", SYST_REGS(scs).SYST_CSR);
        cpu_t *cpu = scs->cpu;

        // create/delete timer if necessary
        if(BITS_ARE_SET(SYST_REGS(scs).SYST_CSR, CSR_ENABLE)){
            // don't create timer if RVR is 0
            uint32_t reload = LOW_BIT32(SYST_REGS(scs).SYST_RVR, 24);
            if(reload == 0){
                return -2;
            }

            timer_t *timer = create_timer(CM_NVIC_VEC_SYSTICK);
            if(timer == NULL){
                return -1;
            }
            retval = add_timer(timer, cpu->timer_list, TRUE);
            start_timer(timer, cpu, reload, systick_do_match);
            // store timer pointer in scs so we can access the timer without searching in the list every time
            scs->systick = timer;
            return retval;
        }else{
            disable_systick(cpu);
        }
    }
    return 0;
}

void set_reload_by_RVR(cpu_t *cpu)
{
    cm_scs_t *scs = (cm_scs_t *)cpu->system_info;
    scs->systick->reload = SYST_REGS(scs).SYST_RVR;
}

int SYST_RVR(uint8_t *data, int rw_flag, cm_scs_t *scs)
{
    if(rw_flag == MEM_READ){
        *(uint32_t *)data = LOW_BIT32(SYST_REGS(scs).SYST_RVR, 24);
    }else{
        SYST_REGS(scs).SYST_RVR = LOW_BIT32(*(uint32_t *)data, 24);
        if(SYST_REGS(scs).SYST_RVR == 0 && scs->systick != NULL){
            // set a flag to disable timer on next wrap
            scs->systick->user_data_int = TRUE;
        }
        // update reload on next wrap
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
            SYST_REGS(scs).SYST_CVR = negative_timer_count(scs->systick, scs->cpu);
        }else{
            SYST_REGS(scs).SYST_CVR = 0;
        }
        *(uint32_t *)data = SYST_REGS(scs).SYST_CVR;
    }else{
        // any write clear the register to 0
        SYST_REGS(scs).SYST_CVR = 0;
        // COUNTFLAG cleared on write to this register
        CLR_BITS(SYST_REGS(scs).SYST_CSR, CSR_COUNTFLAG);
        if(scs->systick != NULL){
            restart_timer(scs->systick, scs->cpu);
        }
        LOG(LOG_DEBUG, "writed SYST_CVR\n");
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
