#include "arm_v7m_ins_implement.h"
#include "error_code.h"
#include "memory_map.h"
#include "cpu.h"
#include "cm_NVIC.h"
#include <assert.h>
#include <stdlib.h>

void sync_banked_register(arm_reg_t *regs, int reg_index)
{
    switch(reg_index){
    case SP_INDEX:
        regs->SP_bank[regs->sp_in_use] = regs->R[reg_index];
        break;
    default:
        break;
    }
}

void restore_banked_register(arm_reg_t *regs, int reg_index)
{
    switch(reg_index){
    case SP_INDEX:
        regs->R[reg_index] = regs->SP_bank[regs->sp_in_use];
    }
}

void DecodeImmShift(uint32_t type, uint32_t imm5, _O uint32_t *shift_t, _O uint32_t *shift_n)
{
    switch(type){
    case 0:
        *shift_t = SRType_LSL;
        *shift_n = imm5;
        break;
    case 1:
        *shift_t = SRType_LSR;
        *shift_n = imm5 == 0 ? 32 : imm5;
        break;
    case 2:
        *shift_t = SRType_ASR;
        *shift_n = imm5 == 0 ? 32 : imm5;
        break;
    case 3:
        if(imm5 == 0){
            *shift_t = SRType_RRX;
            *shift_n = 1;
        }else{
            *shift_t = SRType_ROR;
            *shift_n = imm5;
        }
    }
}

void LSL_C(uint32_t val, int shift, _O uint32_t* result, _O int *carry_out)
{
    *carry_out = 0;
    if(shift > 0){
        *carry_out = val >> (32-shift) & BIT_0;
    }

    *result = val << shift;
}

void LSR_C(uint32_t val, int shift, _O uint32_t* result, _O int *carry_out)
{
    *carry_out = 0;
    if(shift > 0){
        *carry_out = val >> (shift - 1) & BIT_0;
    }

    *result = val >> shift;
}



void ASR_C(uint32_t val, int shift, _O uint32_t* result, _O int *carry_out)
{
    *carry_out = 0;
    if(shift > 0){
        *carry_out = val >> (shift - 1) & BIT_0;
    }
    *result = _ASR32(val, shift);
}

inline uint32_t _ROR32(uint32_t val, int amount)
{
    uint32_t shift_out = val << (32 - amount);
    return val >> amount | shift_out;
}

void ROR_C(uint32_t val, int shift, _O uint32_t* result, _O int *carry_out)
{
    assert(shift != 0);
    *carry_out = val >> (shift - 1) & BIT_0;
    *result = _ROR32(val, shift);
}

void RRX_C(uint32_t val, uint32_t carry_in, _O uint32_t *result, _O int *carry_out)
{
    *result = carry_in << 31 | val >> 1;
    *carry_out = val & BIT_0;
}

void Shift_C(uint32_t val, SRType type, int amount, int carry_in, _O uint32_t* result, _O int *carry_out)
{
    if(amount == 0){
        *result = val;
        *carry_out = carry_in;
        return;
    }

    switch(type){
    case SRType_LSL:
        LSL_C(val, amount, result, carry_out);
        break;
    case SRType_LSR:
        LSR_C(val, amount, result, carry_out);
        break;
    case SRType_ASR:
        ASR_C(val, amount, result, carry_out);
        break;
    case SRType_ROR:
        ROR_C(val, amount, result, carry_out);
        break;
    case SRType_RRX:
        RRX_C(val, carry_in, result, carry_out);
        break;
    default:
        break;
    }
}

inline void Shift(uint32_t val, SRType type, int amount, int carry_in, _O uint32_t* result)
{
    int carry_out;
    Shift_C(val, type, amount, carry_in, result, &carry_out);
}

int ThumbExpandImm_C(uint32_t imm12, uint32_t carry_in, uint32_t *imm32, int*carry_out)
{
    uint32_t imm12_8 = LOW_BIT32(imm12, 8);
    if((imm12 >> 10 & 0x3) == 0){
        switch((imm12 >> 8) & 0x3){
        case 0x0:
            *imm32 = imm12_8;
            break;
        case 0x1:
            if(imm12_8 == 0){
                return -1;
            }else{
                *imm32 = (imm12_8 << 16) | imm12_8;
            }
            break;
        case 0x2:
            if(imm12_8 == 0){
                return -2;
            }else{
                *imm32 = (imm12_8 << 24) | (imm12_8 << 8);
            }
            break;
        case 0x3:
            if(imm12_8 == 0){
                return -3;
            }else{
                *imm32 = (imm12_8 << 24) | (imm12_8 << 16) | (imm12_8 << 8) | imm12_8;
            }
            break;
        default:
            return -4;
        }
        *carry_out = carry_in;
    }else{
        uint32_t unrotated = (1ul << 7) | LOW_BIT32(imm12, 7);
        ROR_C(unrotated, LOW_BIT32(imm12 >> 7, 5), imm32, carry_out);
    }
    return 0;
}

inline uint8_t CurrentCond(arm_reg_t* regs)
{
    return (GET_ITSTATE(regs)>>4) & 0xF;
}

uint8_t ConditionPassed(uint8_t branch_cond, arm_reg_t* regs)
{
    uint8_t cond;
    /* if branch_cond != 0, it means this function was called by conditioned branch instructions.
       Otherwise, it was called by any other instructions. So if it is not in ITblock, it should
       always return passed. */
    if(branch_cond != 0){
        cond = branch_cond;
    }else if(!InITBlock(regs)){
        return 1;
    }else{
        cond = CurrentCond(regs);
    }
    uint8_t result = FALSE;
    switch(cond>>1){
    case 0:
        result = GET_APSR_Z(regs) == 1;    break;
    case 1:
        result = GET_APSR_C(regs) == 1; break;
    case 2:
        result = GET_APSR_N(regs) == 1; break;
    case 3:
        result = GET_APSR_V(regs) == 1; break;
    case 4:
        result = (GET_APSR_C(regs) == 1) && (GET_APSR_Z(regs) == 1); break;
    case 5:
        result = GET_APSR_N(regs) == GET_APSR_V(regs); break;
    case 6:
        result = (GET_APSR_N(regs) == GET_APSR_V(regs)) && (GET_APSR_Z(regs) == 0); break;
    case 7:
        result = 1; break;
    default:
        break;
    }

    if((cond & 0x1) && cond != 0xFF){
        result = !result;
    }

    return result;
}

/*<<ARMv7-M Architecture Reference Manual A2-43>>*/
inline void AddWithCarry(uint32_t op1, uint32_t op2, uint32_t carry_in, uint32_t* result, uint32_t* carry_out, uint32_t *overflow)
{
    /* uint64_t is 64bit */
    if(sizeof(uint64_t) == 8){
        uint64_t unsigned_sum = (uint64_t)op1 + (uint64_t)op2 + (uint64_t)carry_in;
        int64_t op1_64 = (int32_t)op1;
        int64_t op2_64 = (int32_t)op2;
        int64_t signed_sum = op1_64 + op2_64 + (int64_t)carry_in;
        uint32_t _result = unsigned_sum & 0xFFFFFFFFL;
        *overflow = (signed_sum == (int32_t)_result)? 0 : 1;
        *carry_out = (unsigned_sum == (uint64_t)_result) ? 0 : 1;
        *result = _result;

    /* general AddWithCarry if there isn't uint64_t */
    }else{
        // spilit 32 bit number to 2-16bit number
        uint16_t op1_low = op1 & 0xFFFF;
        uint16_t op1_high = (uint32_t)op1 >> 16;
        uint16_t op2_low = op2 & 0xFFFF;
        uint16_t op2_high = (op2) >> 16;

        // calculate low 16 bit carry
        int low_carry;
        uint32_t ulow_sum = (uint32_t)op1_low + (uint32_t)op2_low + (uint32_t)carry_in;
        uint32_t low_result = ulow_sum & 0xFFFF;
        low_carry = (ulow_sum == (uint32_t)low_result) ? 0 : 1;

        // calculate high 16 bit carry and overflow
        uint32_t uhigh_sum = (uint32_t)op1_high + (uint32_t)op2_high + (uint32_t)low_carry;
        int32_t op1_high_32 = (int16_t)op1_high;
        int32_t op2_high_32 = (int16_t)op2_high;
        int32_t shigh_sum = op1_high_32 + op2_high_32 + (int32_t)low_carry;
        uint16_t high_result = uhigh_sum & 0xFFFF;
        *overflow = (shigh_sum == (int16_t)high_result) ? 0 : 1;
        *carry_out = (uhigh_sum == (uint32_t)high_result) ? 0 : 1;
        *result = op1 + op2 + carry_in;
    }
}

/* <<ARMv7-M Architecture Reference Manual 44>> */
void SignedSatQ(int32_t i, int32_t N, int32_t *result, bool_t *saturated)
{
    if(i > ORDER_2(N - 1)){
        *result = (int32_t)ORDER_2(N - 1) - 1;
        *saturated = TRUE;
    }else if(i < -ORDER_2(N - 1)){
        *result = -ORDER_2(N - 1);
        *saturated = TRUE;
    }else{
        *result = i;
        *saturated = FALSE;
    }
    //*result = LOW_BIT32(*result, N);
}

void UnsignedSatQ(int32_t i, int32_t N, uint32_t *result, bool_t *saturated)
{
    if(i > ORDER_2(N - 1)){
        *result = (uint32_t)ORDER_2(N) - 1;
        *saturated = TRUE;
    }else if(i < 0){
        *result = 0;
        *saturated = TRUE;
    }else{
        *result = i;
        *saturated = FALSE;
    }
    //*result = LOW_BIT32(*result, N);
}

/* <<ARMv7-M Architecture Reference Manual 630>> */
inline void BranchTo(uint32_t address, arm_reg_t* regs)
{
    SET_REG_VAL(regs, PC_INDEX, address);
}

/* <<ARMv7-M Architecture Reference Manual 47>> */
inline void BranchWritePC(uint32_t address, arm_reg_t* regs)
{
    BranchTo(DOWN_ALIGN(address, 1), regs);
}

/* <<ARMv7-M Architecture Reference Manual 48>> */
inline void ALUWritePC(uint32_t address, arm_reg_t* regs)
{
    BranchWritePC(address, regs);
}

/* <<ARMv7-M Architecture Reference Manual 47>> */
void BXWritePC(uint32_t address, cpu_t* cpu)
{
    arm_reg_t *regs = ARMv7m_GET_REGS(cpu);
    thumb_state *state = ARMv7m_GET_STATE(cpu);
    uint32_t Tflag;
    int CurrentMode = state->mode;
    if(CurrentMode == MODE_HANDLER &&
        ((address >> 28) & 0xFul) == 0xFul){
        ExceptionReturn(address & 0x00FFFFFF, cpu);
    }else{
        Tflag = address & 0x1ul;
        SET_EPSR_T(regs, Tflag);
        if(Tflag == 0){
            //TODO: UsageFault(Invalid_State);
        }

        BranchTo(DOWN_ALIGN(address, 1), regs);
    }
}

void armv7m_branch(uint32_t address, cpu_t* cpu)
{
    BXWritePC(address, cpu);
}

void armv7m_push(uint32_t reg_val, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;

    uint32_t SP_val = GET_REG_VAL(regs, SP_INDEX);
    SP_val -= 4;
    write_memory(SP_val, (uint8_t*)&reg_val, 4, cpu->memory_map);

    SET_REG_VAL(regs, SP_INDEX, SP_val);
}



/* <<ARMv7-M Architecture Reference Manual 47>> */
inline void LoadWritePC(uint32_t address, cpu_t *cpu)
{
    BXWritePC(address, cpu);
}

/* <<ARMv7-M Architecture Reference Manual B2-694>> */
bool_t FindPriv(arm_reg_t* regs, thumb_state* state)
{
    bool_t ispriv;
    if(state->mode == MODE_HANDLER ||
        (state->mode == MODE_THREAD && GET_CONTROL_PRIV(regs) == 0)){
        ispriv = TRUE;
    }else{
        ispriv = FALSE;
    }
    return ispriv;
}

/* <<ARMv7-M Architecture Reference Manual B2-696>> */
int MemU_with_priv(uint32_t address, int size, _IO uint8_t* buffer, bool_t priv, int type, cpu_t* cpu)
{
    memory_map_t* memory = cpu->memory_map;
    int retval = 0;
    if(0){
        /* TODO: here is the condition of CCR.UNALIGN_TRP == 1*/
        // ExceptionTaken(UsageFault);
        return 0;
    }else{
        /* Since unaligned visit is not different from aligned one in emulation, we just do the samething. */
        if(type == MEM_READ){
            // TODO: Send an io request here and delay the real access to when the instruction finished.
            // send_io_request(address, size, buffer, io_info, type);
            retval = read_memory(address, buffer, size, memory);
            /* reverse big endian */
        }else if(type == MEM_WRITE){
            /* reverse big endian */
            retval = write_memory(address, buffer, size, memory);
        }
    }

    return retval;
}

/* TODO: Memory access is not complete for checking some flags like ALIGN and ENDIANs */
/* <<ARMv7-M Architecture Reference Manual B2-696>> */
int MemU(uint32_t address, int size, _IO uint8_t* buffer, int type, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    thumb_state* state = (thumb_state*)cpu->run_info.cpu_spec_info;
    return MemU_with_priv(address, size, buffer, FindPriv(regs, state), type, cpu);
}

int MemA_with_priv(uint32_t address, int size, _IO uint8_t* buffer, bool_t priv, int type, cpu_t* cpu)
{
    memory_map_t* memory = cpu->memory_map;
    int retval = -1;
    if(address != Align(address, size)){
        //TODO: These registers are memory mapped. So they need to be treated like peripherals.
        /* UFSR.UNALIGENED = '1'
           ExceptionTaken(UsageFault)
         */
        return 0;
    }
    // ValidateAddress() using MPU
    if(type == MEM_READ){
        retval = read_memory(address, buffer, size, memory);
        // if AIRCR.ENDIANNESS == 1 then
        //        value = BigEndianReverse(value, size);
    }else if(type == MEM_WRITE){
        // if AIRCR.ENDIANNESS == 1 then
        // reverse big endian
        retval = write_memory(address, buffer, size, memory);
    }
    return retval;
}

int MemA(uint32_t address, int size, _IO uint8_t* buffer, int type, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    thumb_state* state = (thumb_state*)cpu->run_info.cpu_spec_info;
    return MemA_with_priv(address, size, buffer, FindPriv(regs, state), type, cpu);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-334>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry) = Shift_C(R[m], SRType_LSL, shift_n, APSR.C);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
*************************************/
void _lsl_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    int carry = 0;
    uint32_t result;

    if(!ConditionPassed(0, regs)){
        return;
    }

    Shift_C(GET_REG_VAL(regs, Rm), SRType_LSL, imm, GET_APSR_C(regs), &result, &carry);
    SET_REG_VAL(regs, Rd, result);

    if(setflags != 0){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-338>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry) = Shift_C(R[m], SRType_LSR, shift_n, APSR.C);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
*************************************/
void _lsr_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    int carry = 0;
    uint32_t result;

    if(!ConditionPassed(0, regs)){
        return;
    }

    Shift_C(GET_REG_VAL(regs, Rm), SRType_LSR, imm, GET_APSR_C(regs), &result, &carry);
    SET_REG_VAL(regs, Rd, result);

    if(setflags != 0){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-236>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry) = Shift_C(R[m], SRType_ASR, shift_n, APSR.C);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
*************************************/
void _asr_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    int carry = 0;
    uint32_t result;

    if(!ConditionPassed(0, regs)){
        return;
    }

    Shift_C(GET_REG_VAL(regs, Rm), SRType_ASR, imm, GET_APSR_C(regs), &result, &carry);
    SET_REG_VAL(regs, Rd, result);

    if(setflags != 0){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-405>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry) = Shift_C(R[m], SRType_ROR, shift_n, APSR.C);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
*************************************/
void _ror_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    int carry = 0;
    uint32_t result;

    Shift_C(GET_REG_VAL(regs, Rm), SRType_ROR, imm, GET_APSR_C(regs), &result, &carry);
    SET_REG_VAL(regs, Rd, result);

    if(setflags != 0){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-409>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry) = Shift_C(R[m], SRType_RRX, 1, APSR.C);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
*************************************/
void _rrx(uint32_t Rm, uint32_t Rd, uint32_t setflags, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    int carry = 0;
    uint32_t result;
    Shift_C(GET_REG_VAL(regs, Rm), SRType_RRX, 1, GET_APSR_C(regs), &result, &carry);
    SET_REG_VAL(regs, Rd, result);

    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-224>>
if ConditionPassed() then
    EncodingSpecificOperations();
    shifted = Shift(R[m], shift_t, shift_n, APSR.C);
    (result, carry, overflow) = AddWithCarry(R[n], shifted, '0');
    if d == 15 then
        ALUWritePC(result); // setflags is always FALSE here
    else
        R[d] = result;
        if setflags then
            APSR.N = result<31>;
            APSR.Z = IsZeroBit(result);
            APSR.C = carry;
            APSR.V = overflow;
*************************************/
void _add_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t carry = GET_APSR_C(regs);
    uint32_t overflow;
    uint32_t shifted;
    Shift(GET_REG_VAL(regs, Rm), shift_t, shift_n, carry, &shifted);

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result;
    AddWithCarry(Rn_val, shifted, 0, &result, &carry, &overflow);

    // software won't distinguish PC and other registers like hardware did. Just disable setflags.
    if(Rd == 15){
        ALUWritePC(result, regs);
    }else{
        SET_REG_VAL(regs, Rd, result);
        if(setflags != 0){
            SET_APSR_N(regs, result);
            SET_APSR_Z(regs, result);
            SET_APSR_C(regs, carry);
            SET_APSR_V(regs, overflow);
        }
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-227>>
if ConditionPassed() then
    EncodingSpecificOperations();
    shifted = Shift(R[m], shift_t, shift_n, APSR.C);
    (result, carry, overflow) = AddWithCarry(SP, shifted, ¡®0¡¯);
    if d == 15 then
        ALUWritePC(result); // setflags is always FALSE here
    else
        R[d] = result;
        if setflags then
            APSR.N = result<31>;
            APSR.Z = IsZeroBit(result);
            APSR.C = carry;
            APSR.V = overflow;
*************************************/
void _add_sp_reg(uint32_t Rm, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t carry = GET_APSR_C(regs);
    uint32_t overflow;
    uint32_t shifted;
    Shift(GET_REG_VAL(regs, Rm), shift_t, shift_n, carry, &shifted);

    uint32_t SP_val = GET_REG_VAL(regs, SP_INDEX);
    uint32_t result;
    AddWithCarry(SP_val, shifted, 0, &result, &carry, &overflow);

    // software won't distinguish PC and other registers like hardware did. Just disable setflags.
    if(Rd == 15){
        ALUWritePC(result, regs);
    }else{
        SET_REG_VAL(regs, Rd, result);
        if(setflags != 0){
            SET_APSR_N(regs, result);
            SET_APSR_Z(regs, result);
            SET_APSR_C(regs, carry);
            SET_APSR_V(regs, overflow);
        }
    }
}


/***********************************
<<ARMv7-M Architecture Reference Manual A7-498>>
if ConditionPassed() then
    EncodingSpecificOperations();
    shifted = Shift(R[m], shift_t, shift_n, APSR.C);
    (result, carry, overflow) = AddWithCarry(R[n], NOT(shifted), '1');
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        APSR.V = overflow;
*************************************/
void _sub_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t carry = GET_APSR_C(regs);
    uint32_t overflow;
    uint32_t shifted;
    Shift(GET_REG_VAL(regs, Rm), shift_t, shift_n, carry, &shifted);

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result;
    AddWithCarry(Rn_val, ~shifted, 1, &result, &carry, &overflow);
    SET_REG_VAL(regs, Rd, result);
    if(setflags != 0){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
        SET_APSR_V(regs, overflow);
    }

}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-222>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry, overflow) = AddWithCarry(R[n], imm32, ¡®0¡¯);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        APSR.V = overflow;
*************************************/
void _add_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t carry = GET_APSR_C(regs);
    uint32_t overflow;

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result;
    AddWithCarry(Rn_val, imm32, 0, &result, &carry, &overflow);
    SET_REG_VAL(regs, Rd, result);
    if(setflags != 0){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
        SET_APSR_V(regs, overflow);
    }

}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-496>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry, overflow) = AddWithCarry(R[n], NOT(imm32), ¡®1¡¯);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        APSR.V = overflow;
*************************************/
void _sub_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t carry = GET_APSR_C(regs);
    uint32_t overflow;

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result;
    AddWithCarry(Rn_val, ~imm32, 1, &result, &carry, &overflow);
    SET_REG_VAL(regs, Rd, result);
    if(setflags != 0){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
        SET_APSR_V(regs, overflow);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-348>>
if ConditionPassed() then
    EncodingSpecificOperations();
    result = imm32;
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
*************************************/
void _mov_imm(uint32_t Rd, uint32_t imm32, bool_t setflags, int carry, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t result = imm32;
    SET_REG_VAL(regs, Rd, result);
    if(setflags != 0){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-262>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry, overflow) = AddWithCarry(R[n], NOT(imm32), ¡®1¡¯);
    APSR.N = result<31>;
    APSR.Z = IsZeroBit(result);
    APSR.C = carry;
    APSR.V = overflow;
*************************************/
void _cmp_imm(uint32_t imm32, uint32_t Rn, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t carry = GET_APSR_C(regs);
    uint32_t overflow;

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result;
    AddWithCarry(Rn_val, ~imm32, 1, &result, &carry, &overflow);
    SET_APSR_N(regs, result);
    SET_APSR_Z(regs, result);
    SET_APSR_C(regs, carry);
    SET_APSR_V(regs, overflow);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-233>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (shifted, carry) = Shift_C(R[m], shift_t, shift_n, APSR.C);
    result = R[n] AND shifted;
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
    // APSR.V unchanged
*************************************/
void _and_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    int carry = GET_APSR_C(regs);

    uint32_t shifted;
    Shift_C(GET_REG_VAL(regs, Rm), shift_t, shift_n, carry, &shifted, &carry);

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = Rn_val & shifted;
    SET_REG_VAL(regs, Rd, result);
    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-231>>
if ConditionPassed() then
    EncodingSpecificOperations();
    result = R[n] AND imm32;
    R[d] = result;
        if setflags then
            APSR.N = result<31>;
            APSR.Z = IsZeroBit(result);
            APSR.C = carry;
            // APSR.V unchanged
*************************************/
void _and_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, uint32_t carry, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = Rn_val & imm32;
    SET_REG_VAL(regs, Rd, result);

    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-273>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (shifted, carry) = Shift_C(R[m], shift_t, shift_n, APSR.C);
    result = R[n] EOR shifted;
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
**************************************/
void _eor_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    int carry = GET_APSR_C(regs);
    uint32_t shifted;
    Shift_C(GET_REG_VAL(regs, Rm), shift_t, shift_n, carry, &shifted, &carry);
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = Rn_val^shifted;
    SET_REG_VAL(regs, Rd, result);
    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-271>>
if ConditionPassed() then
    EncodingSpecificOperations();
    result = R[n] EOR imm32;
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
**************************************/
void _eor_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, int carry, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = Rn_val ^ imm32;
    SET_REG_VAL(regs, Rd, result);
    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-273>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (shifted, carry) = Shift_C(R[m], shift_t, shift_n, APSR.C);
    result = R[n] EOR shifted;
    APSR.N = result<31>;
    APSR.Z = IsZeroBit(result);
    APSR.C = carry;
    // APSR.V unchanged
**************************************/
void _teq_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    int carry = GET_APSR_C(regs);
    uint32_t shifted;
    Shift_C(GET_REG_VAL(regs, Rm), shift_t, shift_n, carry, &shifted, &carry);
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = Rn_val ^ shifted;

    SET_APSR_N(regs, result);
    SET_APSR_Z(regs, result);
    SET_APSR_C(regs, carry);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-519>>
if ConditionPassed() then
    EncodingSpecificOperations();
    result = R[n] EOR imm32;
    APSR.N = result<31>;
    APSR.Z = IsZeroBit(result);
    APSR.C = carry;
    // APSR.V unchanged
**************************************/
void _teq_imm(uint32_t imm32, uint32_t Rn, int carry, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = Rn_val ^ imm32;
    SET_APSR_N(regs, result);
    SET_APSR_Z(regs, result);
    SET_APSR_C(regs, carry);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-335>>
if ConditionPassed() then
    EncodingSpecificOperations();
    shift_n = UInt(R[m]<7:0>);
    (result, carry) = Shift_C(R[n], SRType_LSL, shift_n, APSR.C);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
    // APSR.V unchanged
**************************************/
void _lsl_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t shift_n = GET_REG_VAL(regs, Rm) & 0x1F;
    uint32_t result;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    int carry = GET_APSR_C(regs);
    Shift_C(Rn_val, SRType_LSL, shift_n, carry, &result, &carry);
    SET_REG_VAL(regs, Rd, result);

    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}


/***********************************
<<ARMv7-M Architecture Reference Manual A7-339>>
if ConditionPassed() then
    EncodingSpecificOperations();
    shift_n = UInt(R[m]<7:0>);
    (result, carry) = Shift_C(R[n], SRType_LSR, shift_n, APSR.C);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
**************************************/
void _lsr_reg(uint32_t Rm, uint32_t Rn ,uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t shift_n = GET_REG_VAL(regs, Rm) & 0x1F;
    uint32_t result;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    int carry = GET_APSR_C(regs);
    Shift_C(Rn_val, SRType_LSR, shift_n, carry, &result, &carry);
    SET_REG_VAL(regs, Rd, result);

    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-237>>
if ConditionPassed() then
    EncodingSpecificOperations();
    shift_n = UInt(R[m]<7:0>);
    (result, carry) = Shift_C(R[n], SRType_ASR, shift_n, APSR.C);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
**************************************/
void _asr_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t shift_n = GET_REG_VAL(regs, Rm) & 0x1F;
    uint32_t result;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    int carry = GET_APSR_C(regs);
    Shift_C(Rn_val, SRType_ASR, shift_n, carry, &result, &carry);
    SET_REG_VAL(regs, Rd, result);

    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-219>>
if ConditionPassed() then
    EncodingSpecificOperations();
    shifted = Shift(R[m], shift_t, shift_n, APSR.C);
    (result, carry, overflow) = AddWithCarry(R[n], shifted, APSR.C);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        APSR.V = overflow;
**************************************/
void _adc_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t shifted;
    uint32_t carry = GET_APSR_C(regs);
    Shift(GET_REG_VAL(regs, Rm), shift_t, shift_n, carry, &shifted);
    uint32_t result;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t overflow;
    AddWithCarry(Rn_val, shifted, carry, &result, &carry, &overflow);
    SET_REG_VAL(regs, Rd, result);
    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
        SET_APSR_V(regs, overflow);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-217>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry, overflow) = AddWithCarry(R[n], imm32, APSR.C);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        APSR.V = overflow;
**************************************/
void _adc_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t result, carry, overflow;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    AddWithCarry(Rn_val, imm32, GET_APSR_C(regs), &result, &carry, &overflow);
    SET_REG_VAL(regs, Rd, result);
    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
        SET_APSR_V(regs, overflow);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-419>>
if ConditionPassed() then
    EncodingSpecificOperations();
    shifted = Shift(R[m], shift_t, shift_n, APSR.C);
    (result, carry, overflow) = AddWithCarry(R[n], NOT(shifted), APSR.C);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        APSR.V = overflow;
**************************************/
void _sbc_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t shifted;
    uint32_t carry = GET_APSR_C(regs);
    Shift(GET_REG_VAL(regs, Rm), shift_t, shift_n, carry, &shifted);

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result;
    uint32_t overflow;
    AddWithCarry(Rn_val, ~shifted, carry, &result, &carry, &overflow);
    SET_REG_VAL(regs, Rd, result);
    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
        SET_APSR_V(regs, overflow);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-418>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry, overflow) = AddWithCarry(R[n], NOT(imm32), APSR.C);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        APSR.V = overflow;
**************************************/
void _sbc_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t result, carry, overflow;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    AddWithCarry(Rn_val, ~imm32, GET_APSR_C(regs), &result, &carry, &overflow);
    SET_REG_VAL(regs, Rd, result);
    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
        SET_APSR_V(regs, overflow);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-408>>
if ConditionPassed() then
    EncodingSpecificOperations();
    shift_n = UInt(R[m]<7:0>);
    (result, carry) = Shift_C(R[n], SRType_ROR, shift_n, APSR.C);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
**************************************/
void _ror_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t result;
    int carry = GET_APSR_C(regs);
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t shift_n = GET_REG_VAL(regs, Rm) & 0x7F;

    Shift_C(Rn_val, SRType_ROR, shift_n, carry, &result, &carry);
    SET_REG_VAL(regs, Rd, result);
    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-523>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (shifted, carry) = Shift_C(R[m], shift_t, shift_n, APSR.C);
    result = R[n] AND shifted;
    APSR.N = result<31>;
    APSR.Z = IsZeroBit(result);
    APSR.C = carry;
    // APSR.V unchanged
**************************************/
void _tst_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t shifted;
    int carry = GET_APSR_C(regs);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    Shift_C(Rm_val, shift_t, shift_n, carry, &shifted, &carry);

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = Rn_val & shifted;
    SET_APSR_N(regs, result);
    SET_APSR_Z(regs, result);
    SET_APSR_C(regs, carry);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-521>>
if ConditionPassed() then
    EncodingSpecificOperations();
    result = R[n] AND imm32;
    APSR.N = result<31>;
    APSR.Z = IsZeroBit(result);
    APSR.C = carry;
    // APSR.V unchanged
**************************************/
void _tst_imm(uint32_t imm32, uint32_t Rn, uint32_t carry, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = imm32 & Rn_val;
    SET_APSR_N(regs, result);
    SET_APSR_Z(regs, result);
    SET_APSR_C(regs, carry);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-411>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry, overflow) = AddWithCarry(NOT(R[n]), imm32, ¡®1¡¯);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        APSR.V = overflow;
**************************************/
void _rsb_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    /* Warning: this function is not tested since MDK can't generate 16bit code for it */
    uint32_t result, overflow, carry;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    AddWithCarry(~Rn_val, imm32, 1, &result, &carry, &overflow);
    SET_REG_VAL(regs, Rd, result);
    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
        SET_APSR_V(regs, overflow);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-413>>
if ConditionPassed() then
EncodingSpecificOperations();
    shifted = Shift(R[m], shift_t, shift_n, APSR.C);
    (result, carry, overflow) = AddWithCarry(NOT(R[n]), shifted, ¡®1¡¯);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.C = carry;
        APSR.V = overflow;
**************************************/
void _rsb_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t result, overflow, shifted;
    uint32_t carry = GET_APSR_C(regs);
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);

    Shift(GET_REG_VAL(regs, Rm), shift_t, shift_n, carry, &shifted);
    AddWithCarry(~Rn_val, shifted, 1, &result, &carry, &overflow);
    SET_REG_VAL(regs, Rd, result);

    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_C(regs, carry);
        SET_APSR_V(regs, overflow);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-263>>
if ConditionPassed() then
    EncodingSpecificOperations();
    shifted = Shift(R[m], shift_t, shift_n, APSR.C);
    (result, carry, overflow) = AddWithCarry(R[n], NOT(shifted), ¡®1¡¯);
    APSR.N = result<31>;
    APSR.Z = IsZeroBit(result);
    APSR.C = carry;
    APSR.V = overflow;
**************************************/
void _cmp_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t shifted;
    uint32_t carry = GET_APSR_C(regs);
    Shift(GET_REG_VAL(regs, Rm), shift_t, shift_n, carry, &shifted);

    uint32_t result;
    uint32_t overflow;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    AddWithCarry(Rn_val, ~shifted, 1, &result, &carry, &overflow);

    SET_APSR_N(regs, result);
    SET_APSR_Z(regs, result);
    SET_APSR_C(regs, carry);
    SET_APSR_V(regs, overflow);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-259>>
if ConditionPassed() then
    EncodingSpecificOperations();
    shifted = Shift(R[m], shift_t, shift_n, APSR.C);
    (result, carry, overflow) = AddWithCarry(R[n], shifted, ¡®0¡¯);
    APSR.N = result<31>;
    APSR.Z = IsZeroBit(result);
    APSR.C = carry;
    APSR.V = overflow;
**************************************/
void _cmn_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t shifted;
    uint32_t carry = GET_APSR_C(regs);
    Shift(GET_REG_VAL(regs, Rm), shift_t, shift_n, carry, &shifted);

    uint32_t result;
    uint32_t overflow;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    AddWithCarry(Rn_val, shifted, 0, &result, &carry, &overflow);

    SET_APSR_N(regs, result);
    SET_APSR_Z(regs, result);
    SET_APSR_C(regs, carry);
    SET_APSR_V(regs, overflow);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-257>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry, overflow) = AddWithCarry(R[n], imm32, ¡®0¡¯);
    APSR.N = result<31>;
    APSR.Z = IsZeroBit(result);
    APSR.C = carry;
    APSR.V = overflow;
**************************************/
void _cmn_imm(uint32_t imm32, uint32_t Rn, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t result, carry, overflow;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    AddWithCarry(Rn_val, imm32, 0, &result, &carry, &overflow);

    SET_APSR_N(regs, result);
    SET_APSR_Z(regs, result);
    SET_APSR_C(regs, carry);
    SET_APSR_V(regs, overflow);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-373>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (shifted, carry) = Shift_C(R[m], shift_t, shift_n, APSR.C);
    result = R[n] OR shifted;
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
**************************************/
void _orr_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t shifted;
    int32_t carry = GET_APSR_C(regs);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    Shift_C(Rm_val, shift_t, shift_n, carry, &shifted, &carry);

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = Rn_val | shifted;
    SET_REG_VAL(regs, Rd, result);

    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-371>>
if ConditionPassed() then
    EncodingSpecificOperations();
    result = R[n] OR imm32;
    R[d] = result;
        if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
**************************************/
void _orr_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, int carry, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = Rn_val | imm32;
    SET_REG_VAL(regs, Rd, result);

    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-369>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (shifted, carry) = Shift_C(R[m], shift_t, shift_n, APSR.C);
    result = R[n] OR NOT(shifted);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
**************************************/
void _orn_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t shifted;
    int32_t carry = GET_APSR_C(regs);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    Shift_C(Rm_val, shift_t, shift_n, carry, &shifted, &carry);

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = Rn_val | ~shifted;
    SET_REG_VAL(regs, Rd, result);

    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-367>>
if ConditionPassed() then
    EncodingSpecificOperations();
    result = R[n] OR NOT(imm32);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
**************************************/
void _orn_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, int carry, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = Rn_val | ~imm32;
    SET_REG_VAL(regs, Rd, result);

     if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-359>>
if ConditionPassed() then
    EncodingSpecificOperations();
    operand1 = SInt(R[n]); // operand1 = UInt(R[n]) produces the same final results
    operand2 = SInt(R[m]); // operand2 = UInt(R[m]) produces the same final results
    result = operand1 * operand2;
    R[d] = result<31:0>;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        // APSR.C unchanged
        // APSR.V unchanged
**************************************/
void _mul_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    // EncodingSpecificOperations();
    int32_t operand1 = (int32_t)GET_REG_VAL(regs, Rn);
    int32_t operand2 = (int32_t)GET_REG_VAL(regs, Rm);
    int32_t result = operand1 * operand2;

    SET_REG_VAL(regs, Rd, result);
    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-245>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (shifted, carry) = Shift_C(R[m], shift_t, shift_n, APSR.C);
    result = R[n] AND NOT(shifted);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
**************************************/
void _bic_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t shifted;
    int32_t carry = GET_APSR_C(regs);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    Shift_C(Rm_val, shift_t, shift_n, carry, &shifted, &carry);

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = Rn_val & (~shifted);
    SET_REG_VAL(regs, Rd, result);

    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-243>>
if ConditionPassed() then
    EncodingSpecificOperations();
    result = R[n] AND NOT(imm32);
    R[d] = result;
        if setflags then
            APSR.N = result<31>;
            APSR.Z = IsZeroBit(result);
            APSR.C = carry;
            // APSR.V unchanged
**************************************/
void _bic_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, bool_t setflags, int carry, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result = Rn_val & ~imm32;
    SET_REG_VAL(regs, Rd, result);

    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-363>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (shifted, carry) = Shift_C(R[m], shift_t, shift_n, APSR.C);
    result = NOT(shifted);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
**************************************/
void _mvn_reg(uint32_t Rm, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t shifted;
    int32_t carry = GET_APSR_C(regs);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    Shift_C(Rm_val, shift_t, shift_n, carry, &shifted, &carry);

    uint32_t result = ~shifted;
    SET_REG_VAL(regs, Rd, result);

    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-361>>
if ConditionPassed() then
    EncodingSpecificOperations();
    result = NOT(imm32);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        // APSR.V unchanged
**************************************/
void _mvn_imm(uint32_t Rd, uint32_t imm32, bool_t setflags, int carry, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t result = ~imm32;
    SET_REG_VAL(regs, Rd, result);

    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-350>>
if ConditionPassed() then
    EncodingSpecificOperations();
    result = R[m];
    if d == 15 then
        ALUWritePC(result); // setflags is always FALSE here
    else
        R[d] = result;
        if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        // APSR.C unchanged
        // APSR.V unchanged
**************************************/
void _mov_reg(uint32_t Rm, uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t result = Rm_val;
    if(Rd == 15){
        ALUWritePC(result, regs);
    }else{
        SET_REG_VAL(regs, Rd, result);
        if(setflags){
            SET_APSR_N(regs, result);
            SET_APSR_Z(regs, result);
        }
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-352>>
if ConditionPassed() then
    EncodingSpecificOperations();
    R[d]<31:16> = imm16;
    // R[d]<15:0> unchanged
**************************************/
void _movt(uint32_t imm16, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rd_val = GET_REG_VAL(regs, Rd);
    Rd_val &= 0xFFFF;
    Rd_val |= imm16 << 16;
    SET_REG_VAL(regs, Rd, Rd_val);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-461>>
if ConditionPassed() then
    EncodingSpecificOperations();
    operand = Shift(R[n], shift_t, shift_n, APSR.C); // APSR.C ignored
    (result, sat) = SignedSatQ(SInt(operand), saturate_to);
    R[d] = SignExtend(result, 32);
    if sat then
        APSR.Q = ¡®1¡¯;
**************************************/
void _ssat(uint32_t saturate_to, uint32_t Rn, uint32_t Rd, uint32_t shift_n, uint32_t shift_t, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t operand;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    Shift(Rn_val, shift_t, shift_n, GET_APSR_C(regs), &operand);

    int32_t result;
    bool_t sat;
    SignedSatQ((int32_t)operand, saturate_to, &result, &sat);
    SET_REG_VAL(regs, Rd, (uint32_t)result);

    if(sat){
        SET_APSR_Q(regs, 1);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-xxx>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result1, sat1) = SignedSatQ(SInt(R[n]<15:0>), saturate_to);
    (result2, sat2) = SignedSatQ(SInt(R[n]<31:16>), saturate_to);
    R[d]<15:0> = SignExtend(result1, 16);
    R[d]<31:16> = SignExtend(result2, 16);
    if sat1 || sat2 then
        APSR.Q = ¡®1¡¯;
**************************************/
void _ssat16(uint32_t saturate_to, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    int32_t result1; bool_t sat1;
    int32_t result2; bool_t sat2;
    SignedSatQ((int16_t)LOW_BIT32(Rn_val, 16), saturate_to, &result1, &sat1);
    SignedSatQ((int16_t)LOW_BIT32(Rn_val >> 16, 16), saturate_to, &result2, &sat2);
    uint32_t Rd_val = LOW_BIT32(result1, 16) | LOW_BIT32(result2, 16) << 16;
    SET_REG_VAL(regs, Rd, Rd_val);

    if(sat1 || sat2){
        SET_APSR_Q(regs, 1);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-547>>
if ConditionPassed() then
    EncodingSpecificOperations();
    operand = Shift(R[n], shift_t, shift_n, APSR.C); // APSR.C ignored
    (result, sat) = UnsignedSatQ(SInt(operand), saturate_to);
    R[d] = ZeroExtend(result, 32);
    if sat then
        APSR.Q = ¡®1¡¯;
**************************************/
void _usat(uint32_t saturate_to, uint32_t Rn, uint32_t Rd, uint32_t shift_n, uint32_t shift_t, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t operand;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    Shift(Rn_val, shift_t, shift_n, GET_APSR_C(regs), &operand);

    uint32_t result;
    bool_t sat;
    UnsignedSatQ((int32_t)operand, saturate_to, &result, &sat);
    SET_REG_VAL(regs, Rd, (uint32_t)result);

    if(sat){
        SET_APSR_Q(regs, 1);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-549>>
if ConditionPassed() then
EncodingSpecificOperations();
    (result1, sat1) = UnsignedSatQ(SInt(R[n]<15:0>), saturate_to);
    (result2, sat2) = UnsignedSatQ(SInt(R[n]<31:16>), saturate_to);
    R[d]<15:0> = ZeroExtend(result1, 16);
    R[d]<31:16> = ZeroExtend(result2, 16);
    if sat1 || sat2 then
        APSR.Q = ¡®1¡¯;
**************************************/
void _usat16(uint32_t saturate_to, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t result1; bool_t sat1;
    uint32_t result2; bool_t sat2;
    UnsignedSatQ((int16_t)LOW_BIT32(Rn_val, 16), saturate_to, &result1, &sat1);
    UnsignedSatQ((int16_t)LOW_BIT32(Rn_val >> 16, 16), saturate_to, &result2, &sat2);
    uint32_t Rd_val = LOW_BIT32(result1, 16) | LOW_BIT32(result2, 16) << 16;
    SET_REG_VAL(regs, Rd, Rd_val);

    if(sat1 || sat2){
        SET_APSR_Q(regs, 1);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-421>>
if ConditionPassed() then
    EncodingSpecificOperations();
    msbit = lsbit + widthminus1;
    if msbit <= 31 then
        R[d] = SignExtend(R[n]<msbit:lsbit>, 32);
    else
        UNPREDICTABLE;
**************************************/
void _sbfx(uint32_t lsbit, uint32_t widthminus1, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t msbit = lsbit + widthminus1;
    uint32_t result;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    if(msbit < 32){
        result = HIGH_BIT32(Rn_val << (31 - msbit), widthminus1 + 1);
        // NOTE: asr to extend the
        result = _ASR32(result, 31 - widthminus1);
        SET_REG_VAL(regs, Rd, result);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-528>>
if ConditionPassed() then
    EncodingSpecificOperations();
    msbit = lsbit + widthminus1;
    if msbit <= 31 then
        R[d] = ZeroExtend(R[n]<msbit:lsbit>, 32);
    else
        UNPREDICTABLE;
**************************************/
void _ubfx(uint32_t lsbit, uint32_t widthminus1, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t msbit = lsbit + widthminus1;
    uint32_t result;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    if(msbit < 32){
        result = HIGH_BIT32(Rn_val << (31 - msbit), widthminus1 + 1);
        result = result >> (31 - widthminus1);
        SET_REG_VAL(regs, Rd, result);
    }
}

#define BIT_MASK32(low, high) 0xFFFFFFFFul >> low << (low + 31 - high) >> (31 - high)

/***********************************
<<ARMv7-M Architecture Reference Manual A7-242>>
if ConditionPassed() then
    EncodingSpecificOperations();
    if msbit >= lsbit then
        R[d]<msbit:lsbit> = R[n]<(msbit-lsbit):0>;
        // Other bits of R[d] are unchanged
    else
        UNPREDICTABLE;
**************************************/
void _bfi(uint32_t lsbit, uint32_t msbit, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rd_val = GET_REG_VAL(regs, Rd);
    uint32_t mask = BIT_MASK32(lsbit, msbit);
    if(msbit >= lsbit){
        Rd_val &= ~mask;
        Rd_val |= LOW_BIT32(Rn_val, msbit + 1 - lsbit + 1) << lsbit;
        SET_REG_VAL(regs, Rd, Rd_val);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-241>>
if ConditionPassed() then
    EncodingSpecificOperations();
    if msbit >= lsbit then
        R[d]<msbit:lsbit> = Replicate(¡®0¡¯, msbit-lsbit+1);
        // Other bits of R[d] are unchanged
    else
        UNPREDICTABLE;
**************************************/
void _bfc(uint32_t lsbit, uint32_t msbit, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rd_val = GET_REG_VAL(regs, Rd);
    uint32_t mask = BIT_MASK32(lsbit, msbit);
    if(msbit >= lsbit){
        Rd_val &= ~mask;
        SET_REG_VAL(regs, Rd, Rd_val);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-250>>
if ConditionPassed() then
    EncodingSpecificOperations();
    BXWritePC(R[m]);
**************************************/
void _bx(uint32_t Rm, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    BXWritePC(Rm_val, cpu);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-249>>
if ConditionPassed() then
    EncodingSpecificOperations();
    target = R[m];
    next_instr_addr = PC - 2;
    LR = next_instr_addr<31:1> : ¡®1¡¯;
    BLXWritePC(target);
**************************************/
void _blx(uint32_t Rm, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t target = GET_REG_VAL(regs, Rm);
    uint32_t PC_val = GET_REG_VAL(regs, PC_INDEX);
    uint32_t next_instr_addr = PC_val - 2;
    uint32_t LR_val = next_instr_addr | 0x1ul;
    SET_REG_VAL(regs, LR_INDEX, LR_val);
    BXWritePC(target, cpu);
}


/***********************************
<<ARMv7-M Architecture Reference Manual A7-289>>
if ConditionPassed() then
    EncodingSpecificOperations();
    base = Align(PC,4);
    address = if add then (base + imm32) else (base - imm32);
    data = MemU[address,4];
    if t == 15 then
        if address<1:0> == ¡®00¡¯ then LoadWritePC(data); else UNPREDICTABLE;
    else
        R[t] = data;
**************************************/
void _ldr_literal(uint32_t imm32, uint32_t Rt, bool_t add, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t base = GET_REG_VAL(regs, PC_INDEX);
    base = DOWN_ALIGN(base, 2);
    uint32_t address = add ? base+imm32 : base-imm32;

    uint32_t data;
    MemU(address, 4, (uint8_t *)&data, MEM_READ, cpu);
    if(Rt == 15){
        if((address & 0x3ul) == 0){
            LoadWritePC(data, cpu);
        }else{
            LOG(LOG_WARN, "_ldr_literal UNPREDICTABLE\n");
            return;
        }
    }else{
        SET_REG_VAL(regs, Rt, data);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-475>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset = Shift(R[m], shift_t, shift_n, APSR.C);
    address = R[n] + offset;
    data = R[t];
    MemU[address,4] = data;
**************************************/
void _str_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, SRType shift_t, uint32_t shift_n, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    int32_t carry = GET_APSR_C(regs);
    uint32_t offset;
    Shift(Rm_val, shift_t, shift_n, carry, &offset);
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t address = Rn_val + offset;
    uint32_t data = GET_REG_VAL(regs, Rt);
    MemU(address, 4, (uint8_t*)&data, MEM_WRITE, cpu);
}


/***********************************
<<ARMv7-M Architecture Reference Manual A7-491>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset = Shift(R[m], shift_t, shift_n, APSR.C);
    address = R[n] + offset;
    MemU[address,2] = R[t]<15:0>;
**************************************/
void _strh_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, SRType shift_t, uint32_t shift_n, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    int32_t carry = GET_APSR_C(regs);
    uint32_t offset;
    Shift(Rm_val, shift_t, shift_n, carry, &offset);
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t address = Rn_val + offset;
    uint32_t data = GET_REG_VAL(regs, Rt);
    MemU(address, 2, (uint8_t*)&data, MEM_WRITE, cpu);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-479>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset = Shift(R[m], shift_t, shift_n, APSR.C);
    address = R[n] + offset;
    MemU[address,1] = R[t]<7:0>;
**************************************/
void _strb_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, SRType shift_t, uint32_t shift_n, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    int32_t carry = GET_APSR_C(regs);
    uint32_t offset;
    Shift(Rm_val, shift_t, shift_n, carry, &offset);
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t address = Rn_val + offset;
    uint32_t data = GET_REG_VAL(regs, Rt);
    MemU(address, 1, (uint8_t*)&data, MEM_WRITE, cpu);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-321>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset = Shift(R[m], shift_t, shift_n, APSR.C);
    offset_addr = if add then (R[n] + offset) else (R[n] - offset);
    address = if index then offset_addr else R[n];
    R[t] = SignExtend(MemU[address,1], 32);
**************************************/
void _ldrsb_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, bool_t add, bool_t index, SRType shift_t, uint32_t shift_n, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t offset;
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    int32_t carry = GET_APSR_C(regs);
    Shift(Rm_val, shift_t, shift_n, carry, &offset);
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t offset_addr = add ? Rn_val+offset : Rn_val-offset;
    uint32_t address = index ? offset_addr : Rn_val;
    int8_t data;
    MemU(address, 1, (uint8_t*)&data, MEM_READ, cpu);
    SET_REG_VAL(regs, Rt, (int32_t)data);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-291>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset = Shift(R[m], shift_t, shift_n, APSR.C);
    offset_addr = if add then (R[n] + offset) else (R[n] - offset);
    address = if index then offset_addr else R[n];
    data = MemU[address,4];
    if wback then R[n] = offset_addr;
    if t == 15 then
        if address<1:0> == ¡®00¡¯ then LoadWritePC(data); else UNPREDICTABLE;
    else
        R[t] = data;
**************************************/
void _ldr_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, bool_t add, bool_t index, bool_t wback, SRType shift_t, uint32_t shift_n, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t offset;
    uint32_t carry = GET_APSR_C(regs);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    Shift(Rm_val, shift_t, shift_n, carry, &offset);
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t offset_addr = add ? Rn_val+offset : Rn_val-offset;
    uint32_t address = index ? offset_addr : Rn_val;
    uint32_t data;
    MemU(address, 4, (uint8_t *)&data, MEM_READ, cpu);
    if(wback){
        SET_REG_VAL(regs, Rn, offset_addr);
    }
    if(Rt == 15){
        if((address & 0x3ul) == 0){
            LoadWritePC(data, cpu);
        }else{
            LOG(LOG_WARN, "_ldr_reg UNPREDICTABLE\n");
        }
    }else{
        SET_REG_VAL(regs, Rt, data);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-313>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset = Shift(R[m], shift_t, shift_n, APSR.C);
    offset_addr = if add then (R[n] + offset) else (R[n] - offset);
    address = if index then offset_addr else R[n];
    data = MemU[address,2];
    if wback then R[n] = offset_addr;
    R[t] = ZeroExtend(data, 32);
**************************************/
void _ldrh_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, bool_t add, bool_t index, bool_t wback, SRType shift_t, uint32_t shift_n, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t offset;
    uint32_t carry = GET_APSR_C(regs);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    Shift(Rm_val, shift_t, shift_n, carry, &offset);
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t offset_addr = add ? Rn_val+offset : Rn_val-offset;
    uint32_t address = index ? offset_addr : Rn_val;
    uint32_t data = 0;
    MemU(address, 2, (uint8_t *)&data, MEM_READ, cpu);
    if(wback){
        SET_REG_VAL(regs, Rn, offset_addr);
    }
    SET_REG_VAL(regs, Rt, data);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-297>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset = Shift(R[m], shift_t, shift_n, APSR.C);
    offset_addr = if add then (R[n] + offset) else (R[n] - offset);
    address = if index then offset_addr else R[n];
    R[t] = ZeroExtend(MemU[address,1],32);
**************************************/
void _ldrb_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, bool_t add, bool_t index, SRType shift_t, uint32_t shift_n, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t offset;
    uint32_t carry = GET_APSR_C(regs);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    Shift(Rm_val, shift_t, shift_n, carry, &offset);
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t offset_addr = add ? Rn_val+offset : Rn_val-offset;
    uint32_t address = index ? offset_addr : Rn_val;
    uint32_t data = 0;
    MemU(address, 1, (uint8_t *)&data, MEM_READ, cpu);
    SET_REG_VAL(regs, Rt, data);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-329>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset = Shift(R[m], shift_t, shift_n, APSR.C);
    offset_addr = if add then (R[n] + offset) else (R[n] - offset);
    address = if index then offset_addr else R[n];
    data = MemU[address,2];
    if wback then R[n] = offset_addr;
    R[t] = SignExtend(data, 32);
**************************************/
void _ldrsh_reg(uint32_t Rm, uint32_t Rn, uint32_t Rt, bool_t add, bool_t index, bool_t wback, SRType shift_t, uint32_t shift_n, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t offset;
    uint32_t carry = GET_APSR_C(regs);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    Shift(Rm_val, shift_t, shift_n, carry, &offset);
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t offset_addr = add ? Rn_val+offset : Rn_val-offset;
    uint32_t address = index ? offset_addr : Rn_val;
    int16_t data;
    MemU(address, 2, (uint8_t *)&data, MEM_READ, cpu);
    if(wback){
        SET_REG_VAL(regs, Rn, offset_addr);
    }
    SET_REG_VAL(regs, Rt, (int32_t)data);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-473>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset_addr = if add then (R[n] + imm32) else (R[n] - imm32);
    address = if index then offset_addr else R[n];
    MemU[address,4] = R[t];
    if wback then R[n] = offset_addr;
**************************************/
void _str_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt,bool_t add, bool_t index, bool_t wback, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t offset_addr = add ? Rn_val+imm32 : Rn_val-imm32;
    uint32_t address = index ? offset_addr : Rn_val;
    uint32_t data = GET_REG_VAL(regs, Rt);
    MemU(address, 4, (uint8_t *)&data, MEM_WRITE, cpu);
    if(wback){
        SET_REG_VAL(regs, Rn, offset_addr);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-287>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset_addr = if add then (R[n] + imm32) else (R[n] - imm32);
    address = if index then offset_addr else R[n];
    data = MemU[address,4];
    if wback then R[n] = offset_addr;
    if t == 15 then
        if address<1:0> == ¡®00¡¯ then LoadWritePC(data); else UNPREDICTABLE;
    else
        R[t] = data;
**************************************/
void _ldr_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt,bool_t add, bool_t index, bool_t wback, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t offset_addr = add ? Rn_val+imm32 : Rn_val-imm32;
    uint32_t address = index ? offset_addr : Rn_val;
    uint32_t data;
    MemU(address, 4, (uint8_t*)&data, MEM_READ, cpu);
    if(wback){
        SET_REG_VAL(regs, Rn, offset_addr);
    }
    if(Rt == 15){
        if((address & 0x3ul) == 0){
            LoadWritePC(data, cpu);
        }else{
            LOG(LOG_WARN, "_str_imm UNPREDICTABLE\n");
        }
    }else{
        SET_REG_VAL(regs, Rt, data);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-477>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset_addr = if add then (R[n] + imm32) else (R[n] - imm32);
    address = if index then offset_addr else R[n];
    MemU[address,1] = R[t]<7:0>;
    if wback then R[n] = offset_addr;
**************************************/
void _strb_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt,bool_t add, bool_t index, bool_t wback, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t offset_addr = add ? Rn_val+imm32 : Rn_val-imm32;
    uint32_t address = index ? offset_addr : Rn_val;
    uint32_t data = GET_REG_VAL(regs, Rt);
    MemU(address, 1, (uint8_t *)&data, MEM_WRITE, cpu);
    if(wback){
        SET_REG_VAL(regs, Rn, offset_addr);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-293>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset_addr = if add then (R[n] + imm32) else (R[n] - imm32);
    address = if index then offset_addr else R[n];
    R[t] = ZeroExtend(MemU[address,1], 32);
    if wback then R[n] = offset_addr;
**************************************/
void _ldrb_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt,bool_t add, bool_t index, bool_t wback, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t offset_addr = add ? Rn_val+imm32 : Rn_val-imm32;
    uint32_t address = index ? offset_addr : Rn_val;
    uint32_t data = 0;
    MemU(address, 1, (uint8_t*)&data, MEM_READ, cpu);
    SET_REG_VAL(regs, Rt, data);
    if(wback){
        SET_REG_VAL(regs, Rn, offset_addr);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-489>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset_addr = if add then (R[n] + imm32) else (R[n] - imm32);
    address = if index then offset_addr else R[n];
    MemU[address,2] = R[t]<15:0>;
    if wback then R[n] = offset_addr;
**************************************/
void _strh_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt,bool_t add, bool_t index, bool_t wback, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t offset_addr = add ? Rn_val+imm32 : Rn_val-imm32;
    uint32_t address = index ? offset_addr : Rn_val;
    uint32_t data = GET_REG_VAL(regs, Rt);
    MemU(address, 2, (uint8_t *)&data, MEM_WRITE, cpu);
    if(wback){
        SET_REG_VAL(regs, Rn, offset_addr);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-309>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset_addr = if add then (R[n] + imm32) else (R[n] - imm32);
    address = if index then offset_addr else R[n];
    data = MemU[address,2];
    if wback then R[n] = offset_addr;
    R[t] = ZeroExtend(data, 32);
**************************************/
void _ldrh_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt,bool_t add, bool_t index, bool_t wback, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t offset_addr = add ? Rn_val+imm32 : Rn_val-imm32;
    uint32_t address = index ? offset_addr : Rn_val;
    uint32_t data = 0;
    MemU(address, 2, (uint8_t*)&data, MEM_READ, cpu);
    if(wback){
        SET_REG_VAL(regs, Rn, offset_addr);
    }
    SET_REG_VAL(regs, Rt, data);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-229>>
if ConditionPassed() then
    EncodingSpecificOperations();
    result = if add then (Align(PC,4) + imm32) else (Align(PC,4) - imm32);
    R[d] = result;
**************************************/
void _adr(uint32_t imm32, uint32_t Rd, bool_t add, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t PC_val = GET_REG_VAL(regs, PC_INDEX);
    uint32_t result = add ? DOWN_ALIGN(PC_val, 2)+imm32 : DOWN_ALIGN(PC_val, 2)-imm32;
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-225>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry, overflow) = AddWithCarry(SP, imm32, ¡®0¡¯);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        APSR.V = overflow;
**************************************/
void _add_sp_imm(uint32_t imm32, uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t result, carry, overflow;
    uint32_t SP_val = GET_REG_VAL(regs, SP_INDEX);
    AddWithCarry(SP_val, imm32, 0, &result, &carry, &overflow);
    SET_REG_VAL(regs, Rd, result);
    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
        SET_APSR_V(regs, overflow);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-499>>
if ConditionPassed() then
    EncodingSpecificOperations();
    (result, carry, overflow) = AddWithCarry(SP, NOT(imm32), ¡®1¡¯);
    R[d] = result;
    if setflags then
        APSR.N = result<31>;
        APSR.Z = IsZeroBit(result);
        APSR.C = carry;
        APSR.V = overflow;
**************************************/
void _sub_sp_imm(uint32_t imm32, uint32_t Rd, uint32_t setflags, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t result, carry, overflow;
    uint32_t SP_val = GET_REG_VAL(regs, SP_INDEX);
    AddWithCarry(SP_val, ~imm32, 1, &result, &carry, &overflow);
    SET_REG_VAL(regs, Rd, result);
    if(setflags){
        SET_APSR_N(regs, result);
        SET_APSR_Z(regs, result);
        SET_APSR_C(regs, carry);
        SET_APSR_V(regs, overflow);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-251>>
EncodingSpecificOperations();
    if nonzero ^ IsZero(R[n]) then
    BranchWritePC(PC + imm32);
**************************************/
void _cbnz_cbz(uint32_t imm32, uint32_t Rn, uint32_t nonzero, arm_reg_t* regs)
{
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t PC_val = GET_REG_VAL(regs, PC_INDEX);
    if(nonzero ^ (Rn_val == 0)){
        BranchWritePC(PC_val + imm32, regs);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-515>>
if ConditionPassed() then
    EncodingSpecificOperations();
    rotated = ROR(R[m], rotation);
    R[d] = SignExtend(rotated<15:0>, 32);
**************************************/
void _sxth(uint32_t Rm, uint32_t Rd, uint32_t rotation, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t rotated = _ROR32(Rm_val, rotation);
    uint32_t result = (int32_t)((int16_t)LOW_BIT32(rotated, 16));
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-509>>
if ConditionPassed() then
    EncodingSpecificOperations();
    rotated = ROR(R[m], rotation);
    R[d] = R[n] + SignExtend(rotated<15:0>, 32);
**************************************/
void _sxtah(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t rotation, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t rotated = _ROR32(Rm_val, rotation);
    uint32_t result = GET_REG_VAL(regs, Rn) + (int32_t)((int16_t)LOW_BIT32(rotated, 16));
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-513>>
if ConditionPassed() then
EncodingSpecificOperations();
    rotated = ROR(R[m], rotation);
    R[d] = SignExtend(rotated<7:0>, 32);
**************************************/
void _sxtb(uint32_t Rm, uint32_t Rd, uint32_t rotation, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t rotated = _ROR32(Rm_val, rotation);
    uint32_t result = (int32_t)((int8_t)LOW_BIT32(rotated, 8));
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-505>>
if ConditionPassed() then
    EncodingSpecificOperations();
    rotated = ROR(R[m], rotation);
    R[d] = R[n] + SignExtend(rotated<7:0>, 32);
**************************************/
void _sxtab(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t rotation, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t rotated = _ROR32(Rm_val, rotation);
    uint32_t result = GET_REG_VAL(regs, Rn) + (uint32_t)((int8_t)LOW_BIT32(rotated, 8));
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-513>>
if ConditionPassed() then
    EncodingSpecificOperations();
    rotated = ROR(R[m], rotation);
    R[d]<15:0> = SignExtend(rotated<7:0>, 16);
    R[d]<31:16> = SignExtend(rotated<23:16>, 16);
**************************************/
void _sxtb16(uint32_t Rm, uint32_t Rd, uint32_t rotation, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t rotated= _ROR32(Rm_val, rotation);
    uint32_t result = 0;
    result = (uint32_t)((int8_t)LOW_BIT32(rotated, 8)) & 0x0000FFFF;
    result |= (uint32_t)((int8_t)LOW_BIT32(rotated >> 16, 8)) << 16;
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-507>>
if ConditionPassed() then
    EncodingSpecificOperations();
    rotated = ROR(R[m], rotation);
    R[d]<15:0> = R[n]<15:0> + SignExtend(rotated<7:0>, 16);
    R[d]<31:16> = R[n]<31:16> + SignExtend(rotated<23:16>, 16);
**************************************/
void _sxtab16(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t rotation, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t rotated= _ROR32(Rm_val, rotation);
    uint32_t result = 0;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    result = (Rn_val & 0x0000FFFFul) + ((uint32_t)((int8_t)LOW_BIT32(rotated, 8)) & 0x0000FFFF);
    result |= (Rn_val & 0xFFFF0000ul) + ((uint32_t)((int8_t)LOW_BIT32(rotated >> 16, 8)) << 16);
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-563>>
if ConditionPassed() then
    EncodingSpecificOperations();
    rotated = ROR(R[m], rotation);
    R[d] = ZeroExtend(rotated<15:0>, 32);
**************************************/
void _uxth(uint32_t Rm, uint32_t Rd, uint32_t rotation, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t rotated = _ROR32(Rm_val, rotation);
    uint32_t result = rotated & 0x0000FFFFu;
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-557>>
    if ConditionPassed() then
    EncodingSpecificOperations();
    rotated = ROR(R[m], rotation);
    R[d] = R[n] + ZeroExtend(rotated<15:0>, 32);
**************************************/
void _uxtah(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t rotation, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t rotated = _ROR32(Rm_val, rotation);
    uint32_t result = GET_REG_VAL(regs, Rn) + (rotated & 0x0000FFFFu);
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-559>>
if ConditionPassed() then
    EncodingSpecificOperations();
    rotated = ROR(R[m], rotation);
    R[d] = ZeroExtend(rotated<7:0>, 32);
**************************************/
void _uxtb(uint32_t Rm, uint32_t Rd, uint32_t rotation, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t rotated = _ROR32(Rm_val, rotation);
    uint32_t result = LOW_BIT32(rotated, 8);
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-553>>
    if ConditionPassed() then
    EncodingSpecificOperations();
    rotated = ROR(R[m], rotation);
    R[d] = R[n] + ZeroExtend(rotated<7:0>, 32);
**************************************/
void _uxtab(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t rotation, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t rotated = _ROR32(Rm_val, rotation);
    uint32_t result = GET_REG_VAL(regs, Rn) + LOW_BIT32(rotated, 8);
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-561>>
if ConditionPassed() then
    EncodingSpecificOperations();
    rotated = ROR(R[m], rotation);
    R[d]<15:0> = ZeroExtend(rotated<7:0>, 16);
    R[d]<31:16> = ZeroExtend(rotated<23:16>, 16);
**************************************/
void _uxtb16(uint32_t Rm, uint32_t Rd, uint32_t rotation, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t rotated= _ROR32(Rm_val, rotation);
    uint32_t result = 0;
    result = LOW_BIT32(rotated, 8);
    result |= LOW_BIT32(rotated >> 16, 8) << 16;
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-555>>
if ConditionPassed() then
    EncodingSpecificOperations();
    rotated = ROR(R[m], rotation);
    R[d]<15:0> = R[n]<15:0> + ZeroExtend(rotated<7:0>, 16);
    R[d]<31:16> = R[n]<31:16> + ZeroExtend(rotated<23:16>, 16);
**************************************/
void _uxtab16(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t rotation, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t rotated= _ROR32(Rm_val, rotation);
    uint32_t result = 0;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    result = (Rn_val & 0x0000FFFFul) + LOW_BIT32(rotated, 8);
    result |= (Rn_val & 0xFFFF0000ul) + (LOW_BIT32(rotated >> 16, 8) << 16);
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-415>>
if ConditionPassed() then
    EncodingSpecificOperations();
    sum1 = SInt(R[n]<15:0>) + SInt(R[m]<15:0>);
    sum2 = SInt(R[n]<31:16>) + SInt(R[m]<31:16>);
    R[d]<15:0> = sum1<15:0>;
    R[d]<31:16> = sum2<15:0>;
    APSR.GE<1:0> = if sum1 >= 0 then ¡®11¡¯ else ¡®00¡¯;
    APSR.GE<3:2> = if sum2 >= 0 then ¡®11¡¯ else ¡®00¡¯;
**************************************/
void _sadd16(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    int16_t sum1 = (int16_t)LOW_BIT32(Rn_val, 16) + (int16_t)LOW_BIT32(Rm_val, 16);
    int32_t sum2 = (int32_t)HIGH_BIT32(Rn_val, 16) + (int32_t)HIGH_BIT32(Rm_val, 16);
    uint32_t result = LOW_BIT32(sum1, 16) | HIGH_BIT32(sum2, 16);
    SET_REG_VAL(regs, Rd, result);

    SET_APSR_GE16(regs, sum1, sum2);
}


/***********************************
<<ARMv7-M Architecture Reference Manual A7-416>>
if ConditionPassed() then
    EncodingSpecificOperations();
    diff = SInt(R[n]<15:0>) - SInt(R[m]<31:16>);
    sum = SInt(R[n]<31:16>) + SInt(R[m]<15:0>);
    R[d]<15:0> = diff<15:0>;
    R[d]<31:16> = sum<15:0>;
    APSR.GE<1:0> = if diff >= 0 then ¡®11¡¯ else ¡®00¡¯;
    APSR.GE<3:2> = if sum >= 0 then ¡®11¡¯ else ¡®00¡¯;
**************************************/
void _sasx(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    int16_t diff = (int16_t)LOW_BIT32(Rn_val, 16) - (int16_t)LOW_BIT32(Rm_val >> 16, 16);
    int16_t sum = (int16_t)LOW_BIT32(Rn_val >> 16, 16) + (int16_t)LOW_BIT32(Rm_val, 16);
    uint32_t result = LOW_BIT32(diff, 16) | LOW_BIT32(sum, 16) << 16;
    SET_REG_VAL(regs, Rd, result);

    SET_APSR_GE16(regs, diff, sum);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-416>>
if ConditionPassed() then
    EncodingSpecificOperations();
    sum = SInt(R[n]<15:0>) + SInt(R[m]<31:16>);
    diff = SInt(R[n]<31:16>) - SInt(R[m]<15:0>);
    R[d]<15:0> = sum<15:0>;
    R[d]<31:16> = diff<15:0>;
    APSR.GE<1:0> = if sum >= 0 then ¡®11¡¯ else ¡®00¡¯;
    APSR.GE<3:2> = if diff >= 0 then ¡®11¡¯ else ¡®00¡¯;
**************************************/
void _ssax(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    int16_t sum = (int16_t)LOW_BIT32(Rn_val, 16) + (int16_t)LOW_BIT32(Rm_val >> 16, 16);
    int16_t diff = (int16_t)LOW_BIT32(Rn_val >> 16, 16) - (int16_t)LOW_BIT32(Rm_val, 16);
    uint32_t result = LOW_BIT32(sum, 16) | LOW_BIT32(diff, 16) << 16;
    SET_REG_VAL(regs, Rd, result);

    SET_APSR_GE16(regs, sum, diff);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-465>>
if ConditionPassed() then
    EncodingSpecificOperations();
    diff1 = SInt(R[n]<15:0>) - SInt(R[m]<15:0>);
    diff2 = SInt(R[n]<31:16>) - SInt(R[m]<31:16>);
    R[d]<15:0> = diff1<15:0>;
    R[d]<31:16> = diff2<15:0>;
    APSR.GE<1:0> = if diff1 >= 0 then ¡®11¡¯ else ¡®00¡¯;
    APSR.GE<3:2> = if diff2 >= 0 then ¡®11¡¯ else ¡®00¡¯;
**************************************/
void _ssub16(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    int16_t diff1 = (int16_t)LOW_BIT32(Rn_val, 16) - (int16_t)LOW_BIT32(Rm_val, 16);
    int32_t diff2 = (int32_t)HIGH_BIT32(Rn_val, 16) - (int32_t)HIGH_BIT32(Rm_val, 16);
    uint32_t result = LOW_BIT32(diff1, 16) | HIGH_BIT32(diff2, 16);
    SET_REG_VAL(regs, Rd, result);

    SET_APSR_GE16(regs, diff1, diff2);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-416>>
    if ConditionPassed() then
    EncodingSpecificOperations();
    sum1 = SInt(R[n]<7:0>) + SInt(R[m]<7:0>);
    sum2 = SInt(R[n]<15:8>) + SInt(R[m]<15:8>);
    sum3 = SInt(R[n]<23:16>) + SInt(R[m]<23:16>);
    sum4 = SInt(R[n]<31:24>) + SInt(R[m]<31:24>);
    R[d]<7:0> = sum1<7:0>;
    R[d]<15:8> = sum2<7:0>;
    R[d]<23:16> = sum3<7:0>;
    R[d]<31:24> = sum4<7:0>;
    APSR.GE<0> = if sum1 >= 0 then ¡®1¡¯ else ¡®0¡¯;
    APSR.GE<1> = if sum2 >= 0 then ¡®1¡¯ else ¡®0¡¯;
    APSR.GE<2> = if sum3 >= 0 then ¡®1¡¯ else ¡®0¡¯;
    APSR.GE<3> = if sum4 >= 0 then ¡®1¡¯ else ¡®0¡¯;
**************************************/
void _sadd8(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);

    int8_t sum1 = (int8_t)LOW_BIT32(Rn_val, 8) + (int8_t)LOW_BIT32(Rm_val, 8);
    int8_t sum2 = (int8_t)LOW_BIT32(Rn_val >> 8, 8) + (int8_t)LOW_BIT32(Rm_val >> 8, 8);
    int8_t sum3 = (int8_t)LOW_BIT32(Rn_val >> 16, 8) + (int8_t)LOW_BIT32(Rm_val >> 16, 8);
    int8_t sum4 = (int8_t)LOW_BIT32(Rn_val >> 24, 8) + (int8_t)LOW_BIT32(Rm_val >> 24, 8);

    uint32_t result = LOW_BIT32(sum4, 8) << 24 | LOW_BIT32(sum3, 8) << 16 | LOW_BIT32(sum2, 8) << 8  | LOW_BIT32(sum1, 8);
    SET_REG_VAL(regs, Rd, result);

    SET_APSR_GE8(regs, sum1, sum2, sum3, sum4);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-466>>
    if ConditionPassed() then
    EncodingSpecificOperations();
    diff1 = SInt(R[n]<7:0>) - SInt(R[m]<7:0>);
    diff2 = SInt(R[n]<15:8>) - SInt(R[m]<15:8>);
    diff3 = SInt(R[n]<23:16>) - SInt(R[m]<23:16>);
    diff4 = SInt(R[n]<31:24>) - SInt(R[m]<31:24>);
    R[d]<7:0> = diff1<7:0>;
    R[d]<15:8> = diff2<7:0>;
    R[d]<23:16> = diff3<7:0>;
    R[d]<31:24> = diff4<7:0>;
    APSR.GE<0> = if diff1 >= 0 then ¡®1¡¯ else ¡®0¡¯;
    APSR.GE<1> = if diff2 >= 0 then ¡®1¡¯ else ¡®0¡¯;
    APSR.GE<2> = if diff3 >= 0 then ¡®1¡¯ else ¡®0¡¯;
    APSR.GE<3> = if diff4 >= 0 then ¡®1¡¯ else ¡®0¡¯;
**************************************/
void _ssub8(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);

    int8_t diff1 = (int8_t)LOW_BIT32(Rn_val, 8) - (int8_t)LOW_BIT32(Rm_val, 8);
    int8_t diff2 = (int8_t)LOW_BIT32(Rn_val >> 8, 8) - (int8_t)LOW_BIT32(Rm_val >> 8, 8);
    int8_t diff3 = (int8_t)LOW_BIT32(Rn_val >> 16, 8) - (int8_t)LOW_BIT32(Rm_val >> 16, 8);
    int8_t diff4 = (int8_t)LOW_BIT32(Rn_val >> 24, 8) - (int8_t)LOW_BIT32(Rm_val >> 24, 8);

    uint32_t result = LOW_BIT32(diff4, 8) << 24 | LOW_BIT32(diff3, 8) << 16 | LOW_BIT32(diff2, 8) << 8  | LOW_BIT32(diff1, 8);
    SET_REG_VAL(regs, Rd, result);

    SET_APSR_GE8(regs, diff1, diff2, diff3, diff4);
}

/***********************************
TODO: Need to be verified in the real chips
<<ARMv7-M Architecture Reference Manual A7-392>>
if ConditionPassed() then
    EncodingSpecificOperations();
    sum1 = SInt(R[n]<15:0>) + SInt(R[m]<15:0>);
    sum2 = SInt(R[n]<31:16>) + SInt(R[m]<31:16>);
    R[d]<15:0> = SignedSat(sum1, 16);
    R[d]<31:16> = SignedSat(sum2, 16);
**************************************/
void _qadd16(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t sum1 = (int16_t)LOW_BIT32(Rn_val, 16) + (int16_t)LOW_BIT32(Rm_val, 16);
    uint32_t sum2 = (int16_t)LOW_BIT32(Rn_val >> 16, 16) + (int16_t)LOW_BIT32(Rm_val >> 16, 16);

    bool_t dummy;
    int32_t result1, result2;
    SignedSatQ(sum1, 16, &result1, &dummy);
    SignedSatQ(sum2, 16, &result2, &dummy);
    uint32_t result = LOW_BIT32(result1, 16) | LOW_BIT32(result2, 16) << 16;
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
TODO: Need to be verified in the real chips
<<ARMv7-M Architecture Reference Manual A7-394>>
if ConditionPassed() then
    EncodingSpecificOperations();
    diff = SInt(R[n]<15:0>) - SInt(R[m]<31:16>);
    sum = SInt(R[n]<31:16>) + SInt(R[m]<15:0>);
    R[d]<15:0> = SignedSat(diff, 16);
    R[d]<31:16> = SignedSat(sum, 16);
**************************************/
void _qasx(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t diff = (int16_t)LOW_BIT32(Rn_val, 16) - (int16_t)LOW_BIT32(Rm_val >> 16, 16);
    uint32_t sum = (int16_t)LOW_BIT32(Rn_val >> 16, 16) + (int16_t)LOW_BIT32(Rm_val, 16);

    bool_t dummy;
    int32_t result1, result2;
    SignedSatQ(diff, 16, &result1, &dummy);
    SignedSatQ(sum, 16, &result2, &dummy);
    uint32_t result = LOW_BIT32(result1, 16) | LOW_BIT32(result2, 16) << 16;
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
TODO: Need to be verified in the real chips
<<ARMv7-M Architecture Reference Manual A7-397>>
if ConditionPassed() then
    EncodingSpecificOperations();
    sum = SInt(R[n]<15:0>) + SInt(R[m]<31:16>);
    diff = SInt(R[n]<31:16>) - SInt(R[m]<15:0>);
    R[d]<15:0> = SignedSat(sum, 16);
    R[d]<31:16> = SignedSat(diff, 16);
**************************************/
void _qsax(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t sum = (int16_t)LOW_BIT32(Rn_val, 16) + (int16_t)LOW_BIT32(Rm_val >> 16, 16);
    uint32_t diff = (int16_t)LOW_BIT32(Rn_val >> 16, 16) - (int16_t)LOW_BIT32(Rm_val, 16);

    bool_t dummy;
    int32_t result1, result2;
    SignedSatQ(sum, 16, &result1, &dummy);
    SignedSatQ(diff, 16, &result2, &dummy);
    uint32_t result = LOW_BIT32(result1, 16) | LOW_BIT32(result2, 16) << 16;
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
TODO: Need to be verified in the real chips
<<ARMv7-M Architecture Reference Manual A7-399>>
if ConditionPassed() then
    EncodingSpecificOperations();
    diff1 = SInt(R[n]<15:0>) - SInt(R[m]<15:0>);
    diff2 = SInt(R[n]<31:16>) - SInt(R[m]<31:16>);
    R[d]<15:0> = SignedSat(diff1, 16);
    R[d]<31:16> = SignedSat(diff2, 16);
**************************************/
void _qsub16(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t diff1 = (int16_t)LOW_BIT32(Rn_val, 16) - (int16_t)LOW_BIT32(Rm_val, 16);
    uint32_t diff2 = (int16_t)LOW_BIT32(Rn_val >> 16, 16) - (int16_t)LOW_BIT32(Rm_val >> 16, 16);

    bool_t dummy;
    int32_t result1, result2;
    SignedSatQ(diff1, 16, &result1, &dummy);
    SignedSatQ(diff2, 16, &result2, &dummy);
    uint32_t result = LOW_BIT32(result1, 16) | LOW_BIT32(result2, 16) << 16;
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
TODO: Need to be verified in the real chips
<<ARMv7-M Architecture Reference Manual A7-393>>
if ConditionPassed() then
    EncodingSpecificOperations();
    sum1 = SInt(R[n]<7:0>) + SInt(R[m]<7:0>);
    sum2 = SInt(R[n]<15:8>) + SInt(R[m]<15:8>);
    sum3 = SInt(R[n]<23:16>) + SInt(R[m]<23:16>);
    sum4 = SInt(R[n]<31:24>) + SInt(R[m]<31:24>);
    R[d]<7:0> = SignedSat(sum1, 8);
    R[d]<15:8> = SignedSat(sum2, 8);
    R[d]<23:16> = SignedSat(sum3, 8);
    R[d]<31:24> = SignedSat(sum4, 8);
**************************************/
void _qadd8(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t sum1 = (int8_t)LOW_BIT32(Rn_val, 8) + (int8_t)LOW_BIT32(Rm_val, 8);
    uint32_t sum2 = (int8_t)LOW_BIT32(Rn_val >> 8, 8) + (int8_t)LOW_BIT32(Rm_val >> 8, 8);
    uint32_t sum3 = (int8_t)LOW_BIT32(Rn_val >> 16, 8) + (int8_t)LOW_BIT32(Rm_val >> 16, 8);
    uint32_t sum4 = (int8_t)LOW_BIT32(Rn_val >> 24, 8) + (int8_t)LOW_BIT32(Rm_val >> 24, 8);

    bool_t dummy;
    int32_t result[4];
    SignedSatQ(sum1, 8, &result[0], &dummy);
    SignedSatQ(sum2, 8, &result[1], &dummy);
    SignedSatQ(sum3, 8, &result[2], &dummy);
    SignedSatQ(sum4, 8, &result[3], &dummy);
    uint32_t Rd_val = LOW_BIT32(result[0], 8)       | LOW_BIT32(result[1], 8) << 8 |
                      LOW_BIT32(result[2], 8) << 16 | LOW_BIT32(result[3], 8) << 24;
    SET_REG_VAL(regs, Rd, Rd_val);
}

/***********************************
TODO: Need to be verified in the real chips
<<ARMv7-M Architecture Reference Manual A7-400>>
if ConditionPassed() then
    EncodingSpecificOperations();
    diff1 = SInt(R[n]<7:0>) - SInt(R[m]<7:0>);
    diff2 = SInt(R[n]<15:8>) - SInt(R[m]<15:8>);
    diff3 = SInt(R[n]<23:16>) - SInt(R[m]<23:16>);
    diff4 = SInt(R[n]<31:24>) - SInt(R[m]<31:24>);
    R[d]<7:0> = SignedSat(diff1, 8);
    R[d]<15:8> = SignedSat(diff2, 8);
    R[d]<23:16> = SignedSat(diff3, 8);
    R[d]<31:24> = SignedSat(diff4, 8);
**************************************/
void _qsub8(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t diff1 = ((int8_t)LOW_BIT32(Rn_val, 8) - (int8_t)LOW_BIT32(Rm_val, 8));
    uint32_t diff2 = ((int8_t)LOW_BIT32(Rn_val >> 8, 8) - (int8_t)LOW_BIT32(Rm_val >> 8, 8));
    uint32_t diff3 = ((int8_t)LOW_BIT32(Rn_val >> 16, 8) - (int8_t)LOW_BIT32(Rm_val >> 16, 8));
    uint32_t diff4 = ((int8_t)LOW_BIT32(Rn_val >> 24, 8) - (int8_t)LOW_BIT32(Rm_val >> 24, 8));

    bool_t dummy;
    int32_t result[4];
    SignedSatQ(diff1, 8, &result[0], &dummy);
    SignedSatQ(diff2, 8, &result[1], &dummy);
    SignedSatQ(diff3, 8, &result[2], &dummy);
    SignedSatQ(diff4, 8, &result[3], &dummy);
    uint32_t Rd_val = LOW_BIT32(result[0], 8)       | LOW_BIT32(result[1], 8) << 8 |
                      LOW_BIT32(result[2], 8) << 16 | LOW_BIT32(result[3], 8) << 24;
    SET_REG_VAL(regs, Rd, Rd_val);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-427>>
if ConditionPassed() then
    EncodingSpecificOperations();
    sum1 = SInt(R[n]<15:0>) + SInt(R[m]<15:0>);
    sum2 = SInt(R[n]<31:16>) + SInt(R[m]<31:16>);
    R[d]<15:0> = sum1<16:1>;
    R[d]<31:16> = sum2<16:1>;
**************************************/
void _shadd16(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t sum1 = (int32_t)(int16_t)LOW_BIT32(Rn_val, 16) + (int32_t)(int16_t)LOW_BIT32(Rm_val, 16);
    uint32_t sum2 = (int32_t)(int16_t)LOW_BIT32(Rn_val >> 16, 16) + (int32_t)(int16_t)LOW_BIT32(Rm_val >> 16, 16);

    uint32_t result = LOW_BIT32(sum1 >> 1, 16) | LOW_BIT32(sum2 >> 1, 16) << 16;
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-429>>
if ConditionPassed() then
    EncodingSpecificOperations();
    diff = SInt(R[n]<15:0>) - SInt(R[m]<31:16>);
    sum = SInt(R[n]<31:16>) + SInt(R[m]<15:0>);
    R[d]<15:0> = diff<16:1>;
    R[d]<31:16> = sum<16:1>;
**************************************/
void _shasx(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t diff = (int32_t)(int16_t)LOW_BIT32(Rn_val, 16) - (int32_t)(int16_t)LOW_BIT32(Rm_val >> 16, 16);
    uint32_t sum = (int32_t)(int16_t)LOW_BIT32(Rn_val >> 16, 16) + (int32_t)(int16_t)LOW_BIT32(Rm_val, 16);

    uint32_t result = LOW_BIT32(diff >> 1, 16) | LOW_BIT32(sum >> 1, 16) << 16;
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-430>>
if ConditionPassed() then
    EncodingSpecificOperations();
    sum = SInt(R[n]<15:0>) + SInt(R[m]<31:16>);
    diff = SInt(R[n]<31:16>) - SInt(R[m]<15:0>);
    R[d]<15:0> = sum<16:1>;
    R[d]<31:16> = diff<16:1>;
**************************************/
void _shsax(uint32_t Rm, uint32_t Rn, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t sum = (int32_t)(int16_t)LOW_BIT32(Rn_val, 16) + (int32_t)(int16_t)LOW_BIT32(Rm_val >> 16, 16);
    uint32_t diff = (int32_t)(int16_t)LOW_BIT32(Rn_val >> 16, 16) - (int32_t)(int16_t)LOW_BIT32(Rm_val, 16);

    uint32_t result = LOW_BIT32(sum >> 1, 16) | LOW_BIT32(diff >> 1, 16) << 16;
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-389>>
if ConditionPassed() then
    EncodingSpecificOperations();
    address = SP - 4*BitCount(registers);

    for i = 0 to 14
        if registers<i> == ¡®1¡¯ then
            MemA[address,4] = R[i];
            address = address + 4;

    SP = SP - 4*BitCount(registers);
**************************************/
void _push(uint32_t registers, uint32_t bitcount, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t SP_val = GET_REG_VAL(regs, SP_INDEX);
    uint32_t address = SP_val - (bitcount << 2);

    uint32_t data;
    int i;
    for(i = 0; i < 15; i++){
        if(registers & (1ul << i)){
            data = GET_REG_VAL(regs, i);
            MemA(address, 4, (uint8_t*)&data, MEM_WRITE, cpu);
            address += 4;
        }
    }

    SP_val -= bitcount << 2;
    SET_REG_VAL(regs, SP_INDEX, SP_val);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-402>>
if ConditionPassed() then
    EncodingSpecificOperations();
    bits(32) result;
    result<31:24> = R[m]<7:0>;
    result<23:16> = R[m]<15:8>;
    result<15:8> = R[m]<23:16>;
    result<7:0> = R[m]<31:24>;
    R[d] = result;
**************************************/
void _rev(uint32_t Rm, uint32_t Rd, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t result;
    result = ((Rm_val&0x0000FFFF) << 16) | ((Rm_val&0xFFFF0000) >> 16);
    result = ((result&0x00FF00FF) << 8) | ((result&0xFF00FF00) >> 8);
    //result = ((result&0x0F0F0F0F) << 4) | ((result&0xF0F0F0F0) >> 4);
    //result = ((result&0x33333333) << 2) | ((result&0xCCCCCCCC) >> 2);
    //result = ((result&0x55555555) << 1) | ((result&0xAAAAAAAA) >> 1);
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-403>>
if ConditionPassed() then
    EncodingSpecificOperations();
    bits(32) result;
    result<31:24> = R[m]<23:16>;
    result<23:16> = R[m]<31:24>;
    result<15:8> = R[m]<7:0>;
    result<7:0> = R[m]<15:8>;
    R[d] = result;
**************************************/
void _rev16(uint32_t Rm, uint32_t Rd, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t result;
    result = ((Rm_val&0x00FF00FF) << 8) | ((Rm_val&0xFF00FF00) >> 8);
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-404>>
if ConditionPassed() then
    EncodingSpecificOperations();
    bits(32) result;
    result<31:8> = SignExtend(R[m]<7:0>, 24);
    result<7:0> = R[m]<15:8>;
    R[d] = result;
**************************************/
void _revsh(uint32_t Rm, uint32_t Rd, arm_reg_t* regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t result;
    result = ((Rm_val&0x000000FF) << 8) | ((Rm_val&0x0000FF00) >> 8);
    result = (int16_t)result;
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-401>>
if ConditionPassed() then
    EncodingSpecificOperations();
    bits(32) result;
    for i = 0 to 31 do
    result<31-i> = R[m]<i>;
    R[d] = result;
**************************************/
void _rbit(uint32_t Rm, uint32_t Rd, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t result;
    result = ((Rm_val&0x0000FFFF) << 16) | ((Rm_val&0xFFFF0000) >> 16);
    result = ((result&0x00FF00FF) << 8) | ((result&0xFF00FF00) >> 8);
    result = ((result&0x0F0F0F0F) << 4) | ((result&0xF0F0F0F0) >> 4);
    result = ((result&0x33333333) << 2) | ((result&0xCCCCCCCC) >> 2);
    result = ((result&0x55555555) << 1) | ((result&0xAAAAAAAA) >> 1);
    SET_REG_VAL(regs, Rd, result);
}

int count_leading_0(uint32_t val)
{
    int count = 0;
    while(val != 0){
        val >>= 1;
        count++;
    }
    return 32 - count;
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-256>>
if ConditionPassed() then
    EncodingSpecificOperations();
    result = CountLeadingZeroBits(R[m]);
    R[d] = result<31:0>;
**************************************/
void _clz(uint32_t Rm, uint32_t Rd, arm_reg_t *regs)
{
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint32_t result = count_leading_0(Rm_val);
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-387>>
if ConditionPassed() then
    EncodingSpecificOperations();
    address = SP;

    for i = 0 to 14
        if registers<i> == ¡®1¡¯ then
            R[i] = MemA[address,4]; address = address + 4;
    if registers<15> == ¡®1¡¯ then
        LoadWritePC(MemA[address,4]);

    SP = SP + 4*BitCount(registers);
**************************************/
void _pop(uint32_t registers, uint32_t bitcount, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(!ConditionPassed(0, regs)){
        return;
    }

    // EncodingSpecificOperations();
    uint32_t SP_val = GET_REG_VAL(regs, SP_INDEX);
    uint32_t address = SP_val;

    uint32_t data;
    int i;
    for(i = 0; i < 15; i++){
        if(registers & (1ul << i)){
            MemA(address, 4, (uint8_t*)&data, MEM_READ, cpu);
            SET_REG_VAL(regs, i, data);
            address += 4;
        }
    }

    /* Update SP before LoadWritePC */
    SP_val += bitcount << 2;
    SET_REG_VAL(regs, SP_INDEX, SP_val);

    if(registers & (1ul << 15)){
        MemA(address, 4, (uint8_t*)&data, MEM_READ, cpu);
        LoadWritePC(data, cpu);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-387>>
EncodingSpecificOperations();
ITSTATE.IT<7:0> = firstcond:mask;
**************************************/
void _it(uint32_t firstcond, uint32_t mask, arm_reg_t* regs, thumb_state* state)
{
    // EncodingSpecificOperations();
    uint8_t value = (firstcond << 4) | mask;
    state->excuting_IT = TRUE;
    SET_ITSTATE(regs, value);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-469>>
if ConditionPassed() then
    EncodingSpecificOperations();
    address = R[n];

    for i = 0 to 14
        if registers<i> == ¡®1¡¯ then
            if i == n && wback && i != LowestSetBit(registers) then
                MemA[address,4] = bits(32) UNKNOWN; // encoding T1 only
            else
                MemA[address,4] = R[i];
            address = address + 4;
    if wback then R[n] = R[n] + 4*BitCount(registers);
**************************************/
void _stm(uint32_t Rn, uint32_t registers, uint32_t bitcount, bool_t wback, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    if(!ConditionPassed(0, regs)){
        return;
    }
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t address = Rn_val;

    uint32_t data;
    int i;
    for(i = 0; i < 15; i++){
        if(registers & (1ul << i)){
            /*TODO:if(i == Rn && wback == TRUE && i != LowestSetBit(registers)){
                UNKNOW;
            }*/
            /*else*/
            data = GET_REG_VAL(regs, i);
            MemA(address, 4, (uint8_t*)&data, MEM_WRITE, cpu);
            address += 4;
        }
    }

    if(wback){
        Rn_val += bitcount << 2;
        SET_REG_VAL(regs, Rn, Rn_val);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-471>>
if ConditionPassed() then
    EncodingSpecificOperations();
    address = R[n] - 4*BitCount(registers);

    for i = 0 to 14
        if registers<i> == ¡®1¡¯ then
            MemA[address,4] = R[i];
            address = address + 4;

    if wback then R[n] = R[n] - 4*BitCount(registers);
**************************************/
void _stmdb(uint32_t Rn, uint32_t registers, uint32_t bitcount, bool_t wback, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    if(!ConditionPassed(0, regs)){
        return;
    }
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t address = Rn_val - (bitcount << 2);

    uint32_t data;
    int i;
    for(i = 0; i < 15; i++){
        if(registers & (1ul << i)){
            data = GET_REG_VAL(regs, i);
            MemA(address, 4, (uint8_t*)&data, MEM_WRITE, cpu);
            address += 4;
        }
    }

    if(wback){
        Rn_val -= bitcount << 2;
        SET_REG_VAL(regs, Rn, Rn_val);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-283>>
if ConditionPassed() then
    EncodingSpecificOperations();
    address = R[n];

    for i = 0 to 14
        if registers<i> == ¡®1¡¯ then
            R[i] = MemA[address,4]; address = address + 4;
    if registers<15> == ¡®1¡¯ then
        LoadWritePC(MemA[address,4]);

    if wback && registers<n> == ¡®0¡¯ then R[n] = R[n] + 4*BitCount(registers);
**************************************/
void _ldm(uint32_t Rn, uint32_t registers, uint32_t bitcount, bool_t wback, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    if(!ConditionPassed(0, regs)){
        return;
    }
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t address = Rn_val;

    uint32_t data;
    int i;
    for(i = 0; i < 15; i++){
        if(registers & (1ul << i)){
            MemA(address, 4, (uint8_t*)&data, MEM_READ, cpu);
            SET_REG_VAL(regs, i, data);
            address += 4;
        }
    }

    /* Update SP before LoadWrite PC */
    if(wback && ((registers & (1ul << Rn)) == 0)){
        Rn_val += bitcount << 2;
        SET_REG_VAL(regs, Rn, Rn_val);
    }

    if(registers & (1ul << 15)){
        MemA(address, 4, (uint8_t*)&data, MEM_READ, cpu);
        LoadWritePC(data, cpu);
    }

}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-285>>
if ConditionPassed() then
    EncodingSpecificOperations();
    address = R[n] - 4*BitCount(registers);

    for i = 0 to 14
        if registers<i> == ¡®1¡¯ then
            R[i] = MemA[address,4]; address = address + 4;
    if registers<15> == ¡®1¡¯ then
        LoadWritePC(MemA[address,4]);

    if wback && registers<n> == ¡®0¡¯ then R[n] = R[n] - 4*BitCount(registers);
**************************************/
void _ldmdb(uint32_t Rn, uint32_t registers, uint32_t bitcount, bool_t wback, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    if(!ConditionPassed(0, regs)){
        return;
    }
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t address = Rn_val - (bitcount << 2);

    uint32_t data;
    int i;
    for(i = 0; i < 15; i++){
        if(registers & (1ul << i)){
            MemA(address, 4, (uint8_t*)&data, MEM_READ, cpu);
            SET_REG_VAL(regs, i, data);
            address += 4;
        }
    }

    /* Update SP before LoadWrite PC */
    if(wback && ((registers & (1ul << Rn)) == 0)){
        Rn_val -= bitcount << 2;
        SET_REG_VAL(regs, Rn, Rn_val);
    }

    if(registers & (1ul << 15)){
        MemA(address, 4, (uint8_t*)&data, MEM_READ, cpu);
        LoadWritePC(data, cpu);
    }

}

arm_exclusive_t *create_arm_exlusive(uint32_t low_addr, uint32_t high_addr, int cpuid)
{
    arm_exclusive_t *ex = (arm_exclusive_t*)malloc(sizeof(arm_exclusive_t));
    if(ex == NULL){
        goto ex_null;
    }

    ex->cpuid = cpuid;
    ex->low_addr = low_addr;
    ex->high_addr = high_addr;
    return ex;

ex_null:
    return NULL;
}

static int IsExclusiveLocal(uint32_t address, int cpuid, int size, cpu_t *cpu)
{
    thumb_global_state* gstate = ARMv7m_GET_GLOBAL_STATE(cpu);

    list_t *cur;
    arm_exclusive_t *record;
    for_each_list_node(cur, gstate->local_exclusive){
        record = (arm_exclusive_t*)cur->data.pdata;
        if(IN_RANGE(address, record->low_addr, record->high_addr) && record->cpuid == cpuid){
            free(record);
            list_delete(cur);
            return TRUE;
        }
    }

    return FALSE;
}

static int MarkExclusiveLocal(uint32_t address, int cpuid, int size, cpu_t *cpu)
{
    thumb_global_state* gstate = ARMv7m_GET_GLOBAL_STATE(cpu);

    /* find if the address is already makred exlusive */
    list_t *cur;
    arm_exclusive_t *record;
    for_each_list_node(cur, gstate->local_exclusive){
        record = (arm_exclusive_t*)cur->data.pdata;
        if(IN_RANGE(address, record->low_addr, record->high_addr && record->cpuid == cpuid)){
            return SUCCESS;
        }
    }

    /* add the exclusive mark */
    arm_exclusive_t *exdata = create_arm_exlusive(address, address, cpuid);
    if(exdata == NULL){
        goto exdata_null;
    }
    list_t *new_node = list_create_node_ptr(exdata);
    if(new_node == NULL){
        goto node_null;
    }
    list_append(gstate->local_exclusive, new_node);
    return SUCCESS;

node_null:
    free(exdata);
exdata_null:
    return -1;
}

/* B2-698 */
static int ExclusiveMonitorPass(uint32_t address, int size, cpu_t *cpu)
{
    if(address != Align(address, size)){
        return FALSE;
    }else{
        // TODO: MPU
        // memaddresc = ValidateAddress()
    }

   // TODO: int cpuid = ProcessorID();
    int cpuid = 0;
    int passed = IsExclusiveLocal(address, cpuid, size, cpu);
    // TODO: the rest check;

    return passed;
}

static int SetExclusiveMonitor(uint32_t address, int size, cpu_t *cpu)
{
    // TODO: MPU
    // memaddrdesc = memaddrdesc = ValidateAddress(address, FindPriv(), FALSE);
    // if memaddrdesc.memattrs.shareable then
    //      MarkExclusiveGlobal(memaddrdesc.physicaladdress, ProcessorID(), size);

    // TODO: int cpuid = ProcessorID();
    int cpuid = 0;
    return MarkExclusiveLocal(address, cpuid, size, cpu);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-285>>
if ConditionPassed() then
    EncodingSpecificOperations();
    address = R[n] + imm32;
    if ExclusiveMonitorsPass(address,4) then
        MemA[address,4] = R[t];
        R[d] = 0;
    else
        R[d] = 1;
**************************************/
void _strex(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t Rt, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t address = GET_REG_VAL(regs, Rn) + imm32;
    if(ExclusiveMonitorPass(address, 4, cpu)){
        uint32_t Rt_val = GET_REG_VAL(regs, Rt);
        MemA(address, 4, (uint8_t*)&Rt_val, MEM_WRITE, cpu);
        SET_REG_VAL(regs, Rd, 0);
    }else{
        SET_REG_VAL(regs, Rd, 1);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-305>>
if ConditionPassed() then
    EncodingSpecificOperations();
    address = R[n] + imm32;
    SetExclusiveMonitors(address,4);
    R[t] = MemA[address,4];
**************************************/
void _ldrex(uint32_t imm32, uint32_t Rn, uint32_t Rt, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t address = GET_REG_VAL(regs, Rn) + imm32;
    SetExclusiveMonitor(address, 4, cpu);
    uint32_t data;
    MemA(address, 4, (uint8_t*)&data, MEM_READ, cpu);
    SET_REG_VAL(regs, Rt, data);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-483>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset_addr = if add then (R[n] + imm32) else (R[n] - imm32);
    address = if index then offset_addr else R[n];
    MemA[address,4] = R[t];
    MemA[address+4,4] = R[t2];
    if wback then R[n] = offset_addr;
**************************************/
void _strd(uint32_t imm32, uint32_t Rn, uint32_t Rt, uint32_t Rt2, int add, int wback, int index, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t offset_addr = add ? Rn_val + imm32 : Rn_val - imm32;
    uint32_t address = index ? offset_addr : Rn_val;
    uint32_t data = GET_REG_VAL(regs, Rt);
    MemA(address, 4, (uint8_t*)&data, MEM_WRITE, cpu);
    data = GET_REG_VAL(regs, Rt2);
    MemA(address + 4, 4, (uint8_t*)&data, MEM_WRITE, cpu);
    if(wback){
        SET_REG_VAL(regs, Rn, offset_addr);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-301>>
if ConditionPassed() then
    EncodingSpecificOperations();
    offset_addr = if add then (R[n] + imm32) else (R[n] - imm32);
    address = if index then offset_addr else R[n];
    R[t] = MemA[address,4];
    R[t2] = MemA[address+4,4];
    if wback then R[n] = offset_addr;
**************************************/
void _ldrd_imm(uint32_t imm32, uint32_t Rn, uint32_t Rt, uint32_t Rt2, int add, int wback, int index, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t offset_addr = add ? Rn_val + imm32 : Rn_val - imm32;
    uint32_t address = index ? offset_addr : Rn_val;
    uint32_t data;
    MemA(address, 4, (uint8_t*)&data, MEM_READ, cpu);
    SET_REG_VAL(regs, Rt, data);
    MemA(address + 4, 4, (uint8_t*)&data, MEM_READ, cpu);
    SET_REG_VAL(regs, Rt2, data);
    if(wback){
        SET_REG_VAL(regs, Rn, offset_addr);
    }
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-303>>
if ConditionPassed() then
    EncodingSpecificOperations();
    if PC<1:0> != ¡®00¡¯ then UNPREDICTABLE;
    address = if add then (PC + imm32) else (PC - imm32);
    R[t] = MemA[address,4];
    R[t2] = MemA[address+4,4];
**************************************/
void _ldrd_literal(uint32_t imm32,  uint32_t Rt, uint32_t Rt2, int add, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t PC_val = GET_REG_VAL(regs, PC_INDEX);
    if((PC_val & 0x03) != 0){
        LOG(LOG_WARN, "UNPREDICTABLE: _ldrd_literal treated as NOP\n");
        return;
    }

    uint32_t address = add ? PC_val + imm32 : PC_val - imm32;
    uint32_t data;
    MemA(address, 4, (uint8_t*)&data, MEM_READ, cpu);
    SET_REG_VAL(regs, Rt, data);
    MemA(address + 4, 4, (uint8_t*)&data, MEM_READ, cpu);
    SET_REG_VAL(regs, Rt2, data);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-486>>
if ConditionPassed() then
    EncodingSpecificOperations();
    address = R[n];
    if ExclusiveMonitorsPass(address,1) then
        MemA[address,1] = R[t];
        R[d] = 0;
    else
        R[d] = 1;
**************************************/
void _strexb_h(uint32_t Rd, uint32_t Rt, uint32_t Rn, cpu_t *cpu, int size)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t address = GET_REG_VAL(regs, Rn);
    if(ExclusiveMonitorPass(address, size, cpu)){
        uint32_t Rt_val = GET_REG_VAL(regs, Rt);
        MemA(address, size, (uint8_t*)&Rt_val, MEM_WRITE, cpu);
        SET_REG_VAL(regs, Rd, 0);
    }else{
        SET_REG_VAL(regs, Rd, 1);
    }
}

void _strexb(uint32_t Rd, uint32_t Rt, uint32_t Rn, cpu_t *cpu)
{
    _strexb_h(Rd, Rt, Rn, cpu, 1);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-487>>
if ConditionPassed() then
    EncodingSpecificOperations();
    address = R[n];
    if ExclusiveMonitorsPass(address,2) then
        MemA[address,2] = R[t];
        R[d] = 0;
    else
        R[d] = 1;
**************************************/
void _strexh(uint32_t Rd, uint32_t Rt, uint32_t Rn, cpu_t *cpu)
{
    _strexb_h(Rd, Rt, Rn, cpu, 2);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-517>>
if ConditionPassed() then
    EncodingSpecificOperations();
    if is_tbh then
        halfwords = UInt(MemU[R[n]+LSL(R[m],1), 2]);
    else
        halfwords = UInt(MemU[R[n]+R[m], 1]);
    BranchWritePC(PC + 2*halfwords);
**************************************/
void _tbb_h(uint32_t Rn, uint32_t Rm, bool_t is_tbh, cpu_t *cpu)
{
    arm_reg_t *regs = ARMv7m_GET_REGS(cpu);
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    uint32_t Rm_val = GET_REG_VAL(regs, Rm);
    uint16_t halfwords = 0;
    if(is_tbh){
        MemU(Rn_val + (Rm_val << 1), 2, (uint8_t*)&halfwords, MEM_READ, cpu);
    }else{
        MemU(Rn_val + Rm_val, 1, (uint8_t*)&halfwords, MEM_READ, cpu);
    }
    uint32_t PC_val = GET_REG_VAL(regs, PC_INDEX);
    BranchWritePC(PC_val + 2 * halfwords, regs);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-306>>
if ConditionPassed() then
    EncodingSpecificOperations();
    address = R[n];
    SetExclusiveMonitors(address,1);
    R[t] = ZeroExtend(MemA[address,1], 32);
**************************************/
inline void _ldrexb_h(uint32_t Rn, uint32_t Rt, cpu_t *cpu, int size)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    if(!ConditionPassed(0, regs)){
        return;
    }

    uint32_t address = GET_REG_VAL(regs, Rn);
    SetExclusiveMonitor(address, size, cpu);
    uint32_t data = 0;
    MemA(address, size, (uint8_t*)&data, MEM_READ, cpu);
    SET_REG_VAL(regs, Rt, data);
}

void _ldrexb(uint32_t Rn, uint32_t Rt, cpu_t* cpu)
{
    _ldrexb_h(Rn, Rt, cpu, 1);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-307>>
if ConditionPassed() then
    EncodingSpecificOperations();
    address = R[n];
    SetExclusiveMonitors(address,2);
    R[t] = ZeroExtend(MemA[address,2], 32);
**************************************/
void _ldrexh(uint32_t Rn, uint32_t Rt, cpu_t* cpu)
{
    _ldrexb_h(Rn, Rt, cpu, 2);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-375>>
if ConditionPassed() then
    EncodingSpecificOperations();
    operand2 = Shift(R[m], shift_t, shift_n, APSR.C); // APSR.C ignored
    R[d]<15:0> = if tbform then operand2<15:0> else R[n]<15:0>;
    R[d]<31:16> = if tbform then R[n]<31:16> else operand2<31:16>;
**************************************/
void _pkhbt_pkhtb(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t shift_t, uint32_t shift_n, bool_t tbform, arm_reg_t *regs)
{
    if(!ConditionPassed(0, regs)){
       return;
    }

    uint32_t operand2;
    Shift(GET_REG_VAL(regs, Rm), shift_t, shift_n, GET_APSR_C(regs), &operand2);
    uint32_t result;
    uint32_t Rn_val = GET_REG_VAL(regs, Rn);
    result = tbform ? LOW_BIT32(operand2, 16) : LOW_BIT32(Rn_val, 16);
    result |= tbform ? HIGH_BIT32(Rn_val, 16) : HIGH_BIT32(operand2, 16);
    SET_REG_VAL(regs, Rd, result);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-239>>
if ConditionPassed() then
    EncodingSpecificOperations();
    BranchWritePC(PC + imm32);
**************************************/
void _b(int32_t imm32, uint8_t cond, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    if(!ConditionPassed(cond, regs)){
        return;
    }

    uint32_t PC_val = GET_REG_VAL(regs, PC_INDEX);
    BranchWritePC(PC_val+imm32, regs);
}
