#ifndef _ARM_V7M_INS_IMPLEMENT_H_
#define _ARM_V7M_INS_IMPLEMENT_H_

#include "cpu.h"
#include "soc.h"
#include "memory_map.h"
#include "list.h"
#include <stdint.h>
#include <assert.h>
#include "_types.h"

typedef enum{
    SRType_LSL,
    SRType_LSR,
    SRType_ASR,
    SRType_ROR,
    SRType_RRX,
}SRType;

#define MODE_THREAD     1
#define MODE_HANDLER    2


/* used in GET_REG_VAL to get register value while PC_INDEX will get current address of instruction +4
   which is the fact we see when read PC in program and PC_RAW_INDEX will get the address of next
   instruction which is the real PC value in ARM */
#define SP_INDEX 13
#define LR_INDEX 14
#define PC_INDEX 15
#define PC_RAW_INDEX 16

#define BANK_INDEX_MSP 0
#define BANK_INDEX_PSP 1

typedef struct{
    // general registers
    union{
        uint32_t R[32];            // 0x00~0x0D * sizeof(uint32_t)
        struct {
            uint32_t R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12;
            uint32_t SP;
            uint32_t LR;
            uint32_t PC;
            uint32_t F[8];
            uint32_t FPS;
            // control registers
            union{
                uint32_t xPSR;
                uint32_t CPSR;
            };
            uint32_t PRIMASK;
            uint32_t FAULTMASK;
            uint32_t BASEPRI;
            uint32_t CONTROL;
        };
    };

    // banked registers
    uint32_t SP_bank[2];        // backup for MSP/PSP
    int sp_in_use;  /* which is in use (should to be banked) */

    uint32_t PC_return;        /* the return value of PC, it is the same as PC in 32-bit instructions
                               and PC+2 in 16-bit instructions */
}arm_reg_t;

typedef struct thumb_state{
    int excuting_IT;    // the flag of whether in IT block or not
    int mode;           // current working mode
    //int cur_exception;  // current exception
    fifo_t *cur_exception;
}thumb_state;

/* exclusive state */
typedef struct arm_exclusive_t{
    uint32_t low_addr;
    uint32_t high_addr;

    uint32_t local_exclusive[MAX_CPU_NUM];        // store the exclusive address in CPU[id]
    uint32_t global_exclusive;                    // store the global exclusive address;
    bool_t   local_exclusive_enable[MAX_CPU_NUM]; // indicate whether CPU[id] is exclusive
    bool_t   global_exclusive_enable;             // indicate whether the global is exclusive
}arm_exclusive_t;

typedef struct thumb_global_state{
    //list_t *local_exclusive;    // stores arm_exclusive_t
    //list_t *global_exclusive;   // stores arm_exclusive_t

    arm_exclusive_t exclusive_state;
}thumb_global_state;

static inline uint8_t get_bit(uint32_t* reg, uint32_t bit_pos)
{
    return (*reg & bit_pos) == 0 ? 0: 1;
}

// if bit_val != 0 then set bit to 1. bit_pos is defined above
static inline void set_bit(uint32_t* reg, uint32_t bit_pos,int bit_val)
{
    if(bit_val == 0){
        *reg &= ~bit_pos;
    }else{
        *reg |= bit_pos;
    }
}

static inline void set_bits(uint32_t* reg, uint32_t mask, uint32_t offset, uint32_t bits_value)
{
    *reg &= ~(mask << offset);
    *reg |= bits_value << offset;
}

#define HIGH_BIT32(value, bit_num) (assert(bit_num < 32 && bit_num > 0), ((value) & (0xFFFFFFFF << (32-(bit_num)))))
#define LOW_BIT32(value, bit_num) (assert(bit_num < 32 && bit_num > 0), ((value) & (0xFFFFFFFF >> (32-(bit_num)))))
#define LOW_BIT16(value, bit_num) (assert(bit_num < 16 && bit_num > 0), ((value) & (0xFFFF >> (16-(bit_num)))))

#define IN_RANGE(value, low, high) ((value) >= (low) && (value) <= (high))

#define ARMv7m_GET_REGS(cpu) ((arm_reg_t*)(cpu)->regs)
#define ARMv7m_GET_STATE(cpu) ((thumb_state*)(cpu)->run_info.cpu_spec_info)
#define ARMv7m_GET_GLOBAL_STATE(cpu) ((thumb_global_state*)(cpu)->run_info.global_info)

#define PSR_N (0x1UL << 31)
#define PSR_Z (0x1UL << 30)
#define PSR_C (0x1UL << 29)
#define PSR_V (0x1UL << 28)
#define PSR_Q (0x1UL << 27)
#define PSR_T (0x1UL << 24)
#define PSR_GE_MASK 0xFul
#define PSR_GE_OFFSET 16
#define CONTROL_nPRIV (0x1UL)
#define CONTROL_SPSEL (0x1ul << 1)

#define BIT_31 (0x1UL << 31)
#define BIT_1    (0x1UL << 1)
#define BIT_0    (0x1UL)

#define SET_PSR(regs, val) ((regs)->xPSR = (val))
#define SET_APSR(regs, val) ((regs)->xPSR = ((regs)->xPSR & ~0xF8000000ul) | (val))
#define SET_IPSR(regs, val) ((regs)->xPSR = ((regs)->xPSR & ~0x000001FFul) | (val))
#define SET_APSR_N(regs, result_reg) set_bit(&(regs)->xPSR, PSR_N, (result_reg) & BIT_31)
#define SET_APSR_Z(regs, result_reg) set_bit(&(regs)->xPSR, PSR_Z, (result_reg) == 0 ? 1 : 0)
#define SET_APSR_C(regs, carry) set_bit(&(regs)->xPSR, PSR_C, (carry))
#define SET_APSR_V(regs, overflow) set_bit(&(regs)->xPSR, PSR_V, (overflow))
#define SET_APSR_Q(regs, Q) set_bit(&(regs)->xPSR, PSR_Q, (Q))
#define SET_PRIMASK(regs, val) ((regs)->PRIMASK = (val));
#define SET_BASEPRI(regs, val) ((regs)->BASEPRI = (val));
#define SET_FAULTMASK(regs, val) ((regs)->FAULTMASK= (val));
#define SET_APSR_GE16(regs, sum_low, sum_high) do{ \
    uint8_t __ge_1_0 = sum_low >= 0 ? 0x3 : 0; \
    uint8_t __ge_3_2 = sum_high >= 0 ? 0x3 : 0; \
    set_bits(&(regs)->xPSR, PSR_GE_MASK, PSR_GE_OFFSET, __ge_3_2 << 2 | __ge_1_0);\
}while(0)
#define SET_APSR_GE8(regs, sum_low, sum_low2, sum_high2, sum_high) do{\
    uint8_t __ge_0 = sum_low >= 0;\
    uint8_t __ge_1 = sum_low2 >= 0;\
    uint8_t __ge_2 = sum_high2 >= 0;\
    uint8_t __ge_3 = sum_high >= 0;\
    set_bits(&(regs)->xPSR, PSR_GE_MASK, PSR_GE_OFFSET, __ge_3 << 3 | __ge_2 << 2 | __ge_1 << 1 | __ge_0);\
}while(0)
#define SET_EPSR_T(regs, bit) set_bit(&(regs)->xPSR, PSR_T, (bit))

static inline void SET_CONTROL_SPSEL(arm_reg_t *regs, int bit)
{
    set_bit(&regs->CONTROL, CONTROL_SPSEL, bit);
    /* Exchange banked SP */
    regs->SP_bank[regs->sp_in_use] = regs->SP;
    regs->sp_in_use = bit;
    regs->SP = regs->SP_bank[bit];
}
#define SET_CONTROL_nPRIV(regs, bit) set_bit(&(regs)->CONTROL, CONTROL_nPRIV, (bit))

#define GET_PSR(regs) ((regs)->xPSR)
#define GET_APSR(regs) ((regs)->xPSR & 0xF8000000ul)
#define GET_IPSR(regs) ((regs)->xPSR & 0x000001FFul)
#define GET_APSR_N(regs) get_bit(&(regs)->xPSR, PSR_N)
#define GET_APSR_Z(regs) get_bit(&(regs)->xPSR, PSR_Z)
#define GET_APSR_C(regs) get_bit(&(regs)->xPSR, PSR_C)
#define GET_APSR_V(regs) get_bit(&(regs)->xPSR, PSR_V)

#define GET_CONTROL_nPRIV(regs) get_bit(&(regs)->CONTROL, CONTROL_nPRIV)
#define GET_CONTROL_SPSEL(regs) get_bit(&(regs)->CONTROL, CONTROL_SPSEL)
#define GET_CONTROL(regs) ((regs)->CONTROL)
#define GET_PRIMASK(regs) ((regs)->PRIMASK)
#define GET_BASEPRI(regs) ((regs)->BASEPRI)
#define GET_FAULTMASK(regs) ((regs)->FAULTMASK)

#define GET_REG_VAL_UNCON(regs, Rx) (*(&(regs)->R[0]+(Rx)))
#define SET_REG_VAL_UNCON(regs, Rx, val) (*(&(regs)->R[0]+(Rx)) = (val))

#define GET_ITSTATE(regs) ((regs->xPSR >> 8 & 0xFC) | (regs->xPSR >> 25 & 0x3))
#define SET_ITSTATE(regs, value_8bit) \
do{\
    regs->xPSR &= ~((0xFCul << 8)| (0x3ul << 25)); \
    regs->xPSR |= ((value_8bit) & 0xFC) << 8; \
    regs->xPSR |= ((value_8bit) & 0x3) << 25; \
}while(0)

void DecodeImmShift(uint32_t type, uint32_t imm5, Output uint32_t *shift_t, Output uint32_t *shift_n);
int ThumbExpandImm_C(uint32_t imm12, uint32_t carry_in, uint32_t *imm32, int*carry_out);

/* TODO: the real FP extension */
static inline bool_t HaveFPExt(cpu_t *cpu)
{
    return FALSE;
}

static inline uint32_t InITBlock(arm_reg_t* regs)
{
    uint8_t ITstate = GET_ITSTATE(regs);
    return (ITstate & 0xF) != 0;
}

static inline uint32_t LastInITBlock(arm_reg_t* regs)
{
    uint8_t ITstate = GET_ITSTATE(regs);
    return (ITstate & 0xF) == 0x8;
}

static inline void ITAdvance(arm_reg_t* regs)
{
    uint8_t itstat =  GET_ITSTATE(regs);
    uint8_t low4bit = itstat & 0xF;
    if((itstat & 0x7) == 0){
        SET_ITSTATE(regs, 0);
    }else{
        low4bit <<= 1;
        itstat &= ~0xF;
        itstat |= low4bit;
        SET_ITSTATE(regs, itstat);
    }
}

static inline uint8_t check_and_reset_excuting_IT(thumb_state* state)
{
    uint8_t retval = state->excuting_IT;
    state->excuting_IT = 0;
    return retval;
}

#define ORDER_2(order) (1L << (order))
// DOWN_ALIGN(a, n) == Align(a, 2^n)
#define DOWN_ALIGN(val, order) ((val) & (0xFFFFFFFFUL << order))
#define CHECK_PC(PC_val) ((PC_val) & 0x1ul)
static inline uint32_t Align(uint32_t address, uint32_t size)
{
    return address-address%size;
}

static inline uint32_t GET_REG_VAL(arm_reg_t* regs, uint32_t Rx){
    uint32_t val = 0;
    /* PC is special, it always return the value aligned to 4*/
    if(Rx == PC_INDEX){
        val = regs->PC_return;
    }else if(Rx == PC_RAW_INDEX){
        val = regs->PC;
    }else{
        val = GET_REG_VAL_UNCON(regs, Rx);
    }
    return val;
}

static inline void SET_REG_VAL(arm_reg_t* regs, uint32_t Rx, uint32_t val){
    uint32_t modified = val;
    switch(Rx){
    case SP_INDEX:
        modified = DOWN_ALIGN(modified, 2);
        break;
    default:
        break;
    }
    SET_REG_VAL_UNCON(regs, Rx, modified);
}

static inline void SET_REG_VAL_BANKED(arm_reg_t *regs, uint32_t Rx, int banked_index, uint32_t val)
{
    if(Rx == SP_INDEX){
        regs->SP_bank[banked_index] = DOWN_ALIGN(val, 2);
        if(regs->sp_in_use == banked_index){
            SET_REG_VAL(regs, SP_INDEX, val);
        }
    }
}

static inline uint32_t GET_REG_VAL_BANKED(arm_reg_t *regs, uint32_t Rx, int banked_index)
{
    if(Rx == SP_INDEX){
        if(banked_index == regs->sp_in_use){
            return GET_REG_VAL(regs, SP_INDEX);
        }else{
            return regs->SP_bank[banked_index];
        }
    }else{
        LOG(LOG_WARN, "Unknown banked register access\n");
        return 0;
    }
}


static inline uint32_t BitCount32(uint32_t bits)
{
    uint32_t count = 0;
    while(bits != 0){
        bits = bits & (bits-1);
        count++;
    }
    return count;
}

static inline uint32_t _ASR32(uint32_t val, int amount)
{
// MS VC will ASR signed int
#if defined _MSC_VER || defined __GNUC__
     return (uint32_t)((int32_t)val >> amount);
#else
#error Unknow compiler
#endif
}

/* directly operate registers */
void sync_banked_register(arm_reg_t *regs, int reg_index);
void restore_banked_register(arm_reg_t *regs, int reg_index);
int MemA(uint32_t address, int size, IOput uint8_t* buffer, int type, cpu_t* cpu);
void armv7m_branch(uint32_t addr, cpu_t* cpu);
void armv7m_push(uint32_t val, cpu_t* cpu);
int armv7m_get_memory_direct(uint32_t address, int size, Output uint8_t* buffer, cpu_t *cpu);
int armv7m_set_memory_direct(uint32_t address, int size, Output uint8_t* buffer, cpu_t *cpu);

/* implementation of instructions */
void _bkpt(cpu_t *cpu);
void _lsl_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, arm_reg_t* regs);
void _lsr_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, arm_reg_t* regs);
void _asr_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, arm_reg_t* regs);
void _ror_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, arm_reg_t *regs);
void _rrx(uint32_t Rm, uint32_t Rd, uint32_t setflags, arm_reg_t *regs);
void _add_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs);
void _add_sp_reg(uint32_t Rm, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs);
void _sub_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs);
void _add_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, arm_reg_t* regs);
void _sub_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, arm_reg_t* regs);
void _cmp_imm(uint32_t imm32, uint32_t Rn, arm_reg_t* regs);
void _and_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs);
void _and_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, uint32_t carry, arm_reg_t *regs);
void _eor_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs);
void _eor_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, int carry, arm_reg_t *regs);
void _teq_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, arm_reg_t* regs);
void _teq_imm(uint32_t imm32, uint32_t Rn, int carry, arm_reg_t *regs);
void _lsl_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflag, arm_reg_t* regs);
void _lsr_reg(uint32_t Rm, uint32_t Rn ,uint32_t Rd, uint32_t setflag, arm_reg_t* regs);
void _asr_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflag, arm_reg_t* regs);
void _adc_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs);
void _adc_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, arm_reg_t *regs);
void _sbc_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs);
void _sbc_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, arm_reg_t *regs);
void _ror_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflags, arm_reg_t* regs);
void _tst_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, arm_reg_t* regs);
void _tst_imm(uint32_t imm32, uint32_t Rn, uint32_t carry, arm_reg_t *regs);
void _rsb_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, arm_reg_t* regs);
void _rsb_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t *regs);
void _cmp_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, arm_reg_t* regs);
void _cmn_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, arm_reg_t* regs);
void _cmn_imm(uint32_t imm32, uint32_t Rn, arm_reg_t *regs);
void _orr_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs);
void _orr_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, int carry, arm_reg_t *regs);
void _orn_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs);
void _orn_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, int carry, arm_reg_t *regs);
void _mul_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflags, arm_reg_t* regs);
void _mla_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t Ra, bool_t setflags, arm_reg_t *regs);
void _mls_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t Ra, arm_reg_t *regs);
void _smull(uint32_t Rm, uint32_t Rn, uint32_t RdLo, uint32_t RdHi, bool_t setflags, arm_reg_t *regs);
void _umull(uint32_t Rm, uint32_t Rn, uint32_t RdLo, uint32_t RdHi, bool_t setflags, arm_reg_t *regs);
void _smlal(uint32_t Rm, uint32_t Rn, uint32_t RdLo, uint32_t RdHi, bool_t setflags, arm_reg_t *regs);
void _sdiv(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _udiv(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _bic_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs);
void _bic_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, int carry, arm_reg_t *regs);
void _mvn_reg(uint32_t Rm, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs);
void _mvn_imm(uint32_t Rd, uint32_t imm32, bool_t setflags, int carry, arm_reg_t *regs);
void _mov_reg(uint32_t Rm, uint32_t Rd, uint32_t setflag, arm_reg_t* regs);
void _mov_imm(uint32_t Rd, uint32_t imm32, bool_t setflag, int carry, arm_reg_t* regs);
void _movt(uint32_t imm16, uint32_t Rd, arm_reg_t *regs);
void _ssat(uint32_t saturate_to, uint32_t Rn, uint32_t Rd, uint32_t shift_n, uint32_t shift_t, arm_reg_t *regs);
void _ssat16(uint32_t saturate_to, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _usat(uint32_t saturate_to, uint32_t Rn, uint32_t Rd, uint32_t shift_n, uint32_t shift_t, arm_reg_t *regs);
void _usat16(uint32_t saturate_to, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _sbfx(uint32_t lsbit, uint32_t widthminus1, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _ubfx(uint32_t lsbit, uint32_t widthminus1, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _bfi(uint32_t lsbit, uint32_t msbit, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _bfc(uint32_t lsbit, uint32_t msbit, uint32_t Rd, arm_reg_t *regs);
void _bx(uint32_t Rm, cpu_t* cpu);
void _blx(uint32_t Rm, cpu_t* cpu);
void _ldrt(uint32_t imm32, uint32_t Rn, uint32_t Rt, cpu_t *cpu);
void _ldrbt(uint32_t imm32, uint32_t Rn, uint32_t Rt, cpu_t *cpu);
void _ldrsbt(uint32_t imm32, uint32_t Rn, uint32_t Rt, cpu_t *cpu);
void _ldrht(uint32_t imm32, uint32_t Rn, uint32_t Rt, cpu_t *cpu);
void _ldrsht(uint32_t imm32, uint32_t Rn, uint32_t Rt, cpu_t *cpu);
void _ldrb_literal(uint32_t imm32, uint32_t Rt, bool_t add, cpu_t *cpu);
void _ldrsb_literal(uint32_t imm32, uint32_t Rt, bool_t add, cpu_t *cpu);
void _ldrh_literal(uint32_t imm32, uint32_t Rt, bool_t add, cpu_t *cpu);
void _ldrsh_literal(uint32_t imm32, uint32_t Rt, bool_t add, cpu_t *cpu);
void _ldr_literal(uint32_t imm32, uint32_t Rt, bool_t add, cpu_t* cpu);
void _str_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, SRType shift_t, uint32_t shift_n, cpu_t* cpu);
void _strh_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, SRType shift_t, uint32_t shift_n, cpu_t* cpu);
void _strb_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, SRType shift_t, uint32_t shift_n, cpu_t* cpu);
void _ldrsb_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, bool_t add, bool_t index, SRType shift_t, uint32_t shift_n, cpu_t* cpu);
void _ldr_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, bool_t add, bool_t index, bool_t wback, SRType shift_t, uint32_t shift_n, cpu_t* cpu);
void _ldrh_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, bool_t add, bool_t index, bool_t wback, SRType shift_t, uint32_t shift_n, cpu_t* cpu);
void _ldrb_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, bool_t add, bool_t index, SRType shift_t, uint32_t shift_n, cpu_t* cpu);
void _ldrsh_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, bool_t add, bool_t index, bool_t wback, SRType shift_t, uint32_t shift_n, cpu_t* cpu);
void _str_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt,bool_t add, bool_t index, bool_t wback, cpu_t* cpu);
void _ldr_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt,bool_t add, bool_t index, bool_t wback, cpu_t* cpu);
void _strb_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt,bool_t add, bool_t index, bool_t wback, cpu_t* cpu);
void _ldrsb_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt, bool_t add, bool_t index, bool_t wback, cpu_t *cpu);
void _ldrb_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt,bool_t add, bool_t index, bool_t wback, cpu_t* cpu);
void _strh_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt,bool_t add, bool_t index, bool_t wback, cpu_t* cpu);
void _ldrh_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt,bool_t add, bool_t index, bool_t wback, cpu_t* cpu);
void _ldrsh_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt, bool_t add, bool_t index, bool_t wback, cpu_t *cpu);
void _adr(uint32_t imm32, uint32_t Rd, bool_t add, arm_reg_t* regs);
void _add_sp_imm(uint32_t imm32, uint32_t Rd, uint32_t setflags, arm_reg_t* regs);
void _sub_sp_imm(uint32_t imm32, uint32_t Rd, uint32_t setflags, arm_reg_t* regs);
void _cbnz_cbz(uint32_t imm32, uint32_t Rn, uint32_t nonzero, arm_reg_t* regs);
void _sxth(uint32_t Rm, uint32_t Rd, uint32_t rotation, arm_reg_t* regs);
void _sxtah(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t rotation, arm_reg_t *regs);
void _sxtb(uint32_t Rm, uint32_t Rd, uint32_t rotation, arm_reg_t* regs);
void _sxtab(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t rotation, arm_reg_t *regs);
void _sxtb16(uint32_t Rm, uint32_t Rd, uint32_t rotation, arm_reg_t *regs);
void _sxtab16(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t rotation, arm_reg_t *regs);
void _uxth(uint32_t Rm, uint32_t Rd, uint32_t rotation, arm_reg_t* regs);
void _uxtah(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t rotation, arm_reg_t *regs);
void _uxtb(uint32_t Rm, uint32_t Rd, uint32_t rotation, arm_reg_t* regs);
void _uxtab(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t rotation, arm_reg_t *regs);
void _uxtb16(uint32_t Rm, uint32_t Rd, uint32_t rotation, arm_reg_t *regs);
void _uxtab16(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t rotation, arm_reg_t *regs);
void _sadd16(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _sasx(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _ssax(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _sadd8(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _ssub8(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _ssub16(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _qadd16(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _qasx(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _qsax(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _qsub16(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _qadd8(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _qsub8(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _shadd16(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _shasx(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _shsax(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs);
void _push(uint32_t registers, uint32_t bitcount, cpu_t* cpu);
void _rev(uint32_t Rm, uint32_t Rd, arm_reg_t* regs);
void _rev16(uint32_t Rm, uint32_t Rd, arm_reg_t* regs);
void _revsh(uint32_t Rm, uint32_t Rd, arm_reg_t* regs);
void _rbit(uint32_t Rm, uint32_t Rd, arm_reg_t *regs);
void _clz(uint32_t Rm, uint32_t Rd, arm_reg_t *regs);
void _pop(uint32_t registers, uint32_t bitcount, cpu_t* cpu);
void _it(uint32_t firstcond, uint32_t mask, arm_reg_t* regs, thumb_state* state);
void _stm(uint32_t Rn, uint32_t registers, uint32_t bitcount, bool_t wback, cpu_t* cpu);
void _stmdb(uint32_t Rn, uint32_t registers, uint32_t bitcount, bool_t wback, cpu_t* cpu);
void _ldm(uint32_t Rn, uint32_t registers, uint32_t bitcount, bool_t wback, cpu_t* cpu);
void _ldmdb(uint32_t Rn, uint32_t registers, uint32_t bitcount, bool_t wback, cpu_t* cpu);
void _strex(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t Rt, cpu_t* cpu);
void _ldrex(uint32_t imm32, uint32_t Rn, uint32_t Rt, cpu_t* cpu);
void _strd(uint32_t imm32, uint32_t Rn, uint32_t Rt, uint32_t Rt2, int add, int wback, int index, cpu_t* cpu);
void _ldrd_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt, uint32_t Rt2, int add, int wback, int index, cpu_t* cpu);
void _ldrd_literal(uint32_t imm32,  uint32_t Rt, uint32_t Rt2, int add, cpu_t* cpu);
void _strexb(uint32_t Rd, uint32_t Rt, uint32_t Rn, cpu_t *cpu);
void _strexh(uint32_t Rd, uint32_t Rt, uint32_t Rn, cpu_t *cpu);
void _tbb_h(uint32_t Rn, uint32_t Rm, bool_t is_tbh, cpu_t *cpu);
void _ldrexb(uint32_t Rn, uint32_t Rt, cpu_t* cpu);
void _ldrexh(uint32_t Rn, uint32_t Rt, cpu_t* cpu);
void _pkhbt_pkhtb(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t shift_t, uint32_t shift_n, bool_t tbform, arm_reg_t *regs);
void _b(int32_t imm32, uint8_t cond, cpu_t* cpu);
void _bl(int32_t imm32, uint8_t cond, cpu_t *cpu);
void _msr(uint32_t SYSm, uint32_t mask, uint32_t Rn, cpu_t *cpu);
void _mrs(uint32_t SYSm, uint32_t Rd, cpu_t *cpu);
void _clrex(cpu_t *cpu);
#endif
