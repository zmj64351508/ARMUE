#include "_types.h"
#include "arm_v7m_ins_decode.h"
#include "cpu.h"
#include <stdlib.h>
#include <assert.h>

thumb_instruct_table_t *M_translate_table; // table for ARMvX-M
thumb_instruct_table_t *R_translate_table; // table for ARMvX-R, implement in the future
thumb_instruct_table_t *A_translate_table; // table for ARMvX-A, implement in the future



/******                            IMPROTANT                                    */
/****** PC always pointers to the address of next instruction.                */
/****** when 16bit coded, PC += 2. when 32bit coded, PC += 4.                */
/****** But, when 16 bit coded instruction visit PC, it will return PC+2    */
error_code_t set_base_table_value(thumb_decode_t* table, int start, int end, thumb_translate_t value, uint8_t type)
{
    thumb_decode_t decoder;
    int i;
    for(i = start; i <= end; i++){
        decoder.translater = value;
        decoder.type = type;
        table[i] = decoder;
    }

    return SUCCESS;
}
#define set_sub_table_value set_base_table_value

void armv7m_print_reg_val(arm_reg_t* regs)
{
    int i;
    for(i = 0; i < 13; i++){
        printf("R%-3d=0x%08x\n", i, regs->R[i]);
    }

    printf("MSP =0x%08x\n", regs->MSP);
    printf("LR  =0x%08x\n", regs->LR);
    printf("PC  =0x%08x\n", regs->PC);
    printf("xPSR=0x%08x\n\n", regs->xPSR);
}

/****** Here are some sub-parsing functions *******/
/* Most of the sub-parsing function contains the excutable pointer, so we needn't check the type
   of the sub-functions, except `misc 16-bit instructions`. */
thumb_translate16_t shift_add_sub_mov(uint16_t ins_code, cpu_t* cpu)
{
    // call appropriate instruction implement function by inquiring 13~9bit
    return M_translate_table->shift_add_sub_mov_table16[ins_code >> 9 & 0x1F].translater16;
}

thumb_translate16_t data_process(uint16_t ins_code, cpu_t* cpu)
{
    // call appropriate instruction implement function by inquiring 9~6bit
    return M_translate_table->data_process_table16[ins_code >> 6 & 0x0F].translater16;
}

thumb_translate16_t spdata_branch_exchange(uint16_t ins_code, cpu_t* cpu)
{
    // call appropriate instruction implement function by inquiring 9~6bit
    return M_translate_table->spdata_branch_exchange_table16[ins_code >> 6 & 0x0F].translater16;
}

thumb_translate16_t load_store_single(uint16_t ins_code, cpu_t* cpu)
{
    // call appropriate instruction implement function by inquiring 15~9bit
    return M_translate_table->load_store_single_table16[ins_code >> 9 & 0x7F].translater16;
}

thumb_translate16_t misc_16bit_ins(uint16_t ins_code, cpu_t* cpu)
{
    thumb_decode_t decode = M_translate_table->misc_16bit_ins_table16[ins_code >> 5 & 0x7F];
    if(decode.type == THUMB_EXCUTER){
        return decode.translater16;
    }else{
        return (thumb_translate16_t)decode.translater16(ins_code, cpu);
    }
}

thumb_translate16_t con_branch_svc(uint16_t ins_code, cpu_t* cpu)
{
    return M_translate_table->con_branch_svc_table16[ins_code >> 8 & 0x0F].translater16;
}

/****** Here are the final decoders of 16bit instructions ******/
void _lsl_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t imm = ins_code >> 6 & 0x1F;
    uint32_t Rm = ins_code >> 3 & 0x7;
    uint32_t Rd = ins_code & 0x7;
    uint32_t setflags = !InITBlock(regs);

    _lsl_imm(imm, Rm, Rd, setflags, regs);
    LOG_INSTRUCTION("_lsl_imm_16, R%d,R%d,#%d\n", Rd, Rm, imm);
}

void _lsr_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t imm = ins_code >> 6 & 0x1F;
    uint32_t Rm = ins_code >> 3 & 0x7;
    uint32_t Rd = ins_code & 0x7;
    uint32_t setflags = !InITBlock(regs);

    _lsr_imm(imm, Rm, Rd, setflags, regs);
    LOG_INSTRUCTION("_lsr_imm_16, R%d,R%d,#%d\n", Rd, Rm, imm);
}

void _asr_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t imm = ins_code >> 6 & 0x1F;
    uint32_t Rm = ins_code >> 3 & 0x7;
    uint32_t Rd = ins_code & 0x7;
    uint32_t setflags = !InITBlock(regs);

    _asr_imm(imm, Rm, Rd, setflags, regs);
    LOG_INSTRUCTION("_asr_imm_16, R%d,R%d,#%d\n", Rd, Rm, imm);
}

void _add_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rm = ins_code >> 6 & 0x7;
    uint32_t Rn = ins_code >> 3 & 0x7;
    uint32_t Rd = ins_code & 0x7;
    uint32_t setflags = !InITBlock(regs);

    _add_reg(Rm, Rn, Rd, SRType_LSL, 0, setflags, regs);
    LOG_INSTRUCTION("_add_reg_16, R%d,R%d,R%d\n", Rd, Rn, Rm);
}

void _sub_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rm = ins_code >> 6 & 0x7;
    uint32_t Rn = ins_code >> 3 & 0x7;
    uint32_t Rd = ins_code & 0x7;
    uint32_t setflags = !InITBlock(regs);

    _sub_reg(Rm, Rn, Rd, SRType_LSL, 0, setflags, regs);
    LOG_INSTRUCTION("_sub_reg_16, R%d,R%d,R%d\n", Rd, Rn, Rm);
}

void _add_imm3_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t imm32 = ins_code >> 6 & 0x7;
    uint32_t Rn = ins_code >> 3 & 0x7;
    uint32_t Rd = ins_code & 0x7;
    uint32_t setflags = !InITBlock(regs);

    _add_imm(imm32, Rn, Rd, setflags, regs);
    LOG_INSTRUCTION("_add_imm3_16, R%d,R%d,#%d\n", Rd, Rn, imm32);
}

void _sub_imm3_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t imm32 = ins_code >> 6 & 0x7;
    uint32_t Rn = ins_code >> 3 & 0x7;
    uint32_t Rd = ins_code & 0x7;
    uint32_t setflags = !InITBlock(regs);

    _sub_imm(imm32, Rn, Rd, setflags, regs);
    LOG_INSTRUCTION("_add_imm3_16, R%d,R%d,#%d\n", Rd, Rn, imm32);
}

void _mov_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rd = ins_code >> 8 & 0x7;
    uint32_t imm = ins_code & 0xFF;
    uint32_t setflags = !InITBlock(regs);
    uint32_t carry = GET_APSR_C(regs);

    _mov_imm(Rd, imm, setflags, carry, regs);
    LOG_INSTRUCTION("_mov_imm_16, R%d,#%d\n", Rd, imm);
}

void _cmp_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rn = ins_code >> 8 & 0x7;
    uint32_t imm8 = ins_code & 0xFF;

    _cmp_imm(imm8, Rn, regs);
    LOG_INSTRUCTION("_cmp_imm_16, R%d,#%d\n", Rn, imm8);
}

void _add_imm8_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rn = ins_code >> 8 & 0x7;
    uint32_t Rd = Rn;
    uint32_t imm8 = ins_code & 0xFF;
    int setflags = !InITBlock(regs);

    _add_imm(imm8, Rn, Rd, setflags, regs);
    LOG_INSTRUCTION("_add_imm8_16, R%d,#%d\n", Rn, imm8);
}

void _sub_imm8_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rn = ins_code >> 8 & 0x7;
    uint32_t Rd = Rn;
    uint32_t imm8 = ins_code & 0xFF;
    int setflags = !InITBlock(regs);

    _sub_imm(imm8, Rn, Rd, setflags, regs);
    LOG_INSTRUCTION("_sub_imm8_16, R%d,#%d\n", Rn, imm8);
}

/************************/
void _and_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rdn = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;
    int setflags = !InITBlock(regs);

    _and_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
    LOG_INSTRUCTION("_and_reg_16, R%d,R%d\n", Rdn, Rm);
}

void _eor_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rdn = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;
    int setflags = !InITBlock(regs);

    _eor_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
    LOG_INSTRUCTION("_eor_reg_16, R%d,R%d\n", Rdn, Rm);
}

void _lsl_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rdn = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;
    int setflags = !InITBlock(regs);

    _lsl_reg(Rm, Rdn, Rdn, setflags, regs);
    LOG_INSTRUCTION("_lsl_reg_16, R%d,R%d\n", Rdn, Rm);
}

void _lsr_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rdn = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;
    int setflags = !InITBlock(regs);

    _lsr_reg(Rm, Rdn, Rdn, setflags, regs);
    LOG_INSTRUCTION("_lsr_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _asr_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rdn = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;
    int setflags = !InITBlock(regs);

    _asr_reg(Rm, Rdn, Rdn, setflags, regs);
    LOG_INSTRUCTION("_asr_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _adc_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rdn = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;
    int setflags = !InITBlock(regs);

    _adc_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
    LOG_INSTRUCTION("_adc_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _sbc_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rdn = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;
    int setflags = !InITBlock(regs);

    _sbc_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
    LOG_INSTRUCTION("_sbc_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _ror_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rdn = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;
    int setflags = !InITBlock(regs);

    _ror_reg(Rm, Rdn, Rdn, setflags, regs);
    LOG_INSTRUCTION("_ror_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _tst_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rn = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;

    _tst_reg(Rm, Rn, SRType_LSL, 0, regs);
    LOG_INSTRUCTION("_tst_reg_16 R%d,R%d\n", Rn, Rm);
}

void _rsb_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rd = ins_code & 0x7;
    uint32_t Rn = ins_code >> 3 & 0x7;
    int setflags = !InITBlock(regs);
    uint32_t imm32 = 0;

    _rsb_imm(imm32, Rn, Rd, setflags, regs);
    LOG_INSTRUCTION("_rsb_imm_16 R%d,R%d,#0\n", Rd, Rn);
}

void _cmp_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rn = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;

    _cmp_reg(Rm, Rn, SRType_LSL, 0, regs);
    LOG_INSTRUCTION("_cmp_reg_16 R%d,R%d\n", Rn, Rm);
}

void _cmn_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rn = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;

    _cmn_reg(Rm, Rn, SRType_LSL, 0, regs);
    LOG_INSTRUCTION("_cmn_reg_16 R%d,R%d\n", Rn, Rm);
}

void _orr_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rdn = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;
    int setflags = !InITBlock(regs);

    _orr_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
    LOG_INSTRUCTION("_orr_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _mul_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rdm = ins_code & 0x7;
    uint32_t Rn = ins_code >> 3 & 0x7;
    int setflags = !InITBlock(regs);

    _mul_reg(Rdm, Rn, Rdm, setflags, regs);
    LOG_INSTRUCTION("_mul_reg_16 R%d,R%d,R%d\n", Rdm, Rn, Rdm);
}

void _bic_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rdn = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;
    int setflags = !InITBlock(regs);

    _bic_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
    LOG_INSTRUCTION("_bic_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _mvn_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rd = ins_code & 0x7;
    uint32_t Rm = ins_code >> 3 & 0x7;
    int setflags = !InITBlock(regs);

    _mvn_reg(Rm, Rd, SRType_LSL, 0, setflags, regs);
    LOG_INSTRUCTION("_mvn_reg_16 R%d,R%d\n", Rd, Rm);
}

void _add_sp_reg_T1(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t DM = ins_code >> 7 & 0x1ul;
    uint32_t Rdm = (DM << 3) | (ins_code & 0x7ul);

    if(Rdm == 15 && InITBlock(regs) && !LastInITBlock(regs)){
        LOG_INSTRUCTION("_add_sp_reg_T1 R%d,SP,R%d as UNPREDICTABLE\n", Rdm, Rdm);
    }else{
        _add_sp_reg(Rdm, Rdm, SRType_LSL, 0, 0, regs);
        LOG_INSTRUCTION("_add_sp_reg_T1 R%d,SP,R%d\n", Rdm, Rdm);
    }
}

void _add_sp_reg_T2(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rm = ins_code >> 3 & 0xFul;
    uint32_t Rd = SP_INDEX;
    assert(Rm != 13);

    _add_sp_reg(Rm, Rd, SRType_LSL, 0, 0, regs);
    LOG_INSTRUCTION("_add_sp_reg_T1 SP,R%d\n", Rm);
}

void _add_reg_spec_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t DN = ins_code >> 7 & 0x1ul;
    uint32_t Rdn = (DN << 3) | (ins_code & 0x7ul);
    uint32_t Rm = ins_code >> 3 & 0xFul;

    if(Rm == 13){
        _add_sp_reg_T1(ins_code, cpu);
    }else if(Rdn == 13){
        _add_sp_reg_T2(ins_code, cpu);
    }else{
        /* unpredictable */
        if((Rdn == 15 && InITBlock(regs) && !LastInITBlock(regs)) ||
           (Rdn == 15 && Rm == 15)){
            LOG_INSTRUCTION("_add_reg_spec_16 R%d,R%d as UNPREDICTABLE\n", Rdn, Rm);
        }else{
            _add_reg(Rm, Rdn, Rdn, SRType_LSL, 0, 0, regs);
            LOG_INSTRUCTION("_add_reg_spec_16 R%d,R%d\n", Rdn, Rm);
        }
    }
}

void _cmp_reg_spec_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t DN = ins_code >> 7 & 0x1ul;
    uint32_t Rn = (DN << 3) | (ins_code & 0x7ul);
    uint32_t Rm = ins_code >> 3 & 0xFul;

    /* unpredictable */
    if((Rn < 8 && Rm < 8) ||
       (Rn == 15 && Rm == 15)){
        LOG_INSTRUCTION("_cmp_reg_spec_16 R%d,R%d as UNPREDICTABLE\n", Rn, Rm);
    }else{
        _cmp_reg(Rm, Rn, SRType_LSL, 0, regs);
        LOG_INSTRUCTION("_cmp_reg_spec_16 R%d,R%d\n", Rn, Rm);
    }
}

void _mov_reg_spec_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t DN = ins_code >> 7 & 0x1ul;
    uint32_t Rd = (DN << 3) | (ins_code & 0x7ul);
    uint32_t Rm = ins_code >> 3 & 0xFul;
    uint32_t setflag = 0;

    if(Rd == 15 && InITBlock(regs) && !LastInITBlock(regs)){
        LOG_INSTRUCTION("_mov_reg_spec_16 PC,R%d as UNPREDICTABLE\n", Rm);
    }else{
        _mov_reg(Rm, Rd, setflag, regs);
        LOG_INSTRUCTION("_mov_reg_16 R%d,R%d\n", Rd, Rm);
    }
}

void _bx_spec_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rm = ins_code >> 3 & 0xFul;
    if((ins_code & 0x7ul) != 0 ||
       (InITBlock(regs) && !LastInITBlock(regs))){
        LOG_INSTRUCTION("_bx_spec_16 R%d as UNPREDICTABLE\n", Rm);
    }else{
        _bx(Rm, cpu);
        LOG_INSTRUCTION("_bx_spec_16 R%d\n", Rm);
    }
}

void _blx_spec_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rm = ins_code >> 3 & 0xFul;
    if((ins_code & 0x7ul) != 0 ||
       (InITBlock(regs) && !LastInITBlock(regs))){
        LOG_INSTRUCTION("_blx_spec_16 R%d as UNPREDICTABLE\n", Rm);
    }else{
        _blx(Rm, cpu);
        LOG_INSTRUCTION("_blx_spec_16 R%d\n", Rm);
    }
}

void _ldr_literal_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t imm8 = (ins_code & 0xFFul) << 2;
    uint32_t Rt = ins_code >> 8 & 0x7ul;
    bool_t add = TRUE;

    _ldr_literal(imm8, Rt, add, cpu);
    LOG_INSTRUCTION("_ldr_literal_16 R%d,[PC,#%d]\n", Rt, imm8);
}

inline void decode_ldr_str_reg_16(uint16_t ins_code, _O uint32_t* Rm, _O uint32_t* Rt, _O uint32_t* Rn)
{
    *Rt = LOW_BIT16(ins_code, 3);
    *Rn = LOW_BIT16(ins_code>>3, 3);
    *Rm = LOW_BIT16(ins_code>>6, 3);
}

void _str_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t Rt, Rn, Rm;
    decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);
    _str_reg(Rm, Rn, Rt, SRType_LSL, 0, cpu);
    LOG_INSTRUCTION("_str_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _strh_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t Rt, Rn, Rm;
    decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);

    _strh_reg(Rm, Rn, Rt, SRType_LSL, 0, cpu);
    LOG_INSTRUCTION("_strh_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _strb_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t Rt, Rn, Rm;
    decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);

    _strb_reg(Rm, Rn, Rt, SRType_LSL, 0, cpu);
    LOG_INSTRUCTION("_strb_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _ldrsb_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t Rt, Rn, Rm;
    decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);

    bool_t add = TRUE;
    bool_t index = TRUE;
    _ldrsb_reg(Rm, Rn, Rt, add, index, SRType_LSL, 0, cpu);
    LOG_INSTRUCTION("_ldrsb_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _ldr_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t Rt, Rn, Rm;
    decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);
    bool_t index = TRUE;
    bool_t add = TRUE;
    bool_t wback = FALSE;

    _ldr_reg(Rm, Rn, Rt, add, index, wback, SRType_LSL, 0, cpu);
    LOG_INSTRUCTION("_ldr_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _ldrh_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t Rt, Rn, Rm;
    decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);
    bool_t index = TRUE;
    bool_t add = TRUE;
    bool_t wback = FALSE;

    _ldrh_reg(Rm, Rn, Rt, add, index, wback, SRType_LSL, 0, cpu);
    LOG_INSTRUCTION("_ldrh_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _ldrb_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t Rt, Rn, Rm;
    decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);
    bool_t index = TRUE;
    bool_t add = TRUE;

    _ldrb_reg(Rm, Rn, Rt, add, index, SRType_LSL, 0, cpu);
    LOG_INSTRUCTION("_ldrb_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _ldrsh_reg_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t Rt, Rn, Rm;
    decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);
    bool_t index = TRUE;
    bool_t add = TRUE;
    bool_t wback = FALSE;

    _ldrsh_reg(Rm, Rn, Rt, add, index, wback, SRType_LSL, 0, cpu);
    LOG_INSTRUCTION("_ldrb_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

inline void decode_ldr_str_imm_16(uint16_t ins_code, _O uint32_t* imm, _O uint32_t* Rt, _O uint32_t* Rn)
{
    *Rt = LOW_BIT16(ins_code, 3);
    *Rn = LOW_BIT16(ins_code>>3, 3);
    *imm = LOW_BIT16(ins_code>>6, 5);
}

void _str_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t imm5, Rt, Rn;
    decode_ldr_str_imm_16(ins_code, &imm5, &Rt, &Rn);
    bool_t add = TRUE;
    bool_t index = TRUE;
    bool_t wback = FALSE;

    _str_imm(imm5, Rn, Rt, add, index, wback, cpu);
    LOG_INSTRUCTION("_str_imm_16 R%d,[R%d,#0x%d]\n", Rt, Rn, imm5);
}

void _ldr_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t imm5, Rt, Rn;
    decode_ldr_str_imm_16(ins_code, &imm5, &Rt, &Rn);
    bool_t add = TRUE;
    bool_t index = TRUE;
    bool_t wback = FALSE;

    _ldr_imm(imm5, Rn, Rt, add, index, wback, cpu);
    LOG_INSTRUCTION("_ldr_imm_16 R%d,[R%d,#0x%d]\n", Rt, Rn, imm5);
}

void _strb_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t imm5, Rt, Rn;
    decode_ldr_str_imm_16(ins_code, &imm5, &Rt, &Rn);
    bool_t add = TRUE;
    bool_t index = TRUE;
    bool_t wback = FALSE;

    _strb_imm(imm5, Rn, Rt, add, index, wback, cpu);
    LOG_INSTRUCTION("_strb_imm_16 R%d,[R%d,#0x%d]\n", Rt, Rn, imm5);
}

void _ldrb_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t imm5, Rt, Rn;
    decode_ldr_str_imm_16(ins_code, &imm5, &Rt, &Rn);
    bool_t add = TRUE;
    bool_t index = TRUE;
    bool_t wback = FALSE;

    _ldrb_imm(imm5, Rn, Rt, add, index, wback, cpu);
    LOG_INSTRUCTION("_ldrb_imm_16 R%d,[R%d,#0x%d]\n", Rt, Rn, imm5);
}

void _strh_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t imm5, Rt, Rn;
    decode_ldr_str_imm_16(ins_code, &imm5, &Rt, &Rn);
    bool_t add = TRUE;
    bool_t index = TRUE;
    bool_t wback = FALSE;

    _strh_imm(imm5, Rn, Rt, add, index, wback, cpu);
    LOG_INSTRUCTION("_strh_imm_16 R%d,[R%d,#0x%d]\n", Rt, Rn, imm5);
}

void _ldrh_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t imm5, Rt, Rn;
    decode_ldr_str_imm_16(ins_code, &imm5, &Rt, &Rn);
    bool_t add = TRUE;
    bool_t index = TRUE;
    bool_t wback = FALSE;

    _ldrh_imm(imm5, Rn, Rt, add, index, wback, cpu);
    LOG_INSTRUCTION("_ldrh_imm_16 R%d,[R%d,#0x%d]\n", Rt, Rn, imm5);
}

inline void decode_ldr_str_sp_imm_16(uint16_t ins_code, _O uint32_t* imm, _O uint32_t* Rt)
{
    *imm = LOW_BIT16(ins_code, 8);
    *Rt = LOW_BIT16(ins_code>>8, 3);
}

void _str_sp_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t imm8, Rt;
    uint32_t Rn = 13;
    decode_ldr_str_sp_imm_16(ins_code, &imm8, &Rt);

    bool_t index = TRUE;
    bool_t add = TRUE;
    bool_t wback = FALSE;

    _str_imm(imm8, Rn, Rt, add, index, wback, cpu);
    LOG_INSTRUCTION("_str_sp_imm_16 R%d,[SP,#0x%d]\n", Rt, imm8);
}

void _ldr_sp_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t imm8, Rt;
    uint32_t Rn = 13;
    decode_ldr_str_sp_imm_16(ins_code, &imm8, &Rt);

    bool_t index = TRUE;
    bool_t add = TRUE;
    bool_t wback = FALSE;

    _ldr_imm(imm8, Rn, Rt, add, index, wback, cpu);
    LOG_INSTRUCTION("_ldr_sp_imm_16 R%d,[SP,#0x%d]\n", Rt, imm8);
}

void _adr_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t imm8 = LOW_BIT16(ins_code, 8) << 2;
    uint32_t Rd = LOW_BIT16(ins_code>>8, 3);
    bool_t add = TRUE;

    _adr(imm8, Rd, add, regs);
    LOG_INSTRUCTION("_adr_16 R%d,#%d\n", Rd, imm8);
}

void _add_sp_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t imm8 = LOW_BIT16(ins_code, 8) << 2;
    uint32_t Rd = LOW_BIT16(ins_code>>8, 3);
    uint32_t setflags = FALSE;

    _add_sp_imm(imm8, Rd, setflags, regs);
    LOG_INSTRUCTION("_add_sp_imm_16 R%d,sp,#%d\n", Rd, imm8);
}

void _add_sp_sp_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t imm7 = LOW_BIT16(ins_code, 7) << 2;
    uint32_t Rd = 13;
    uint32_t setflags = FALSE;

    _add_sp_imm(imm7, Rd, setflags, regs);
    LOG_INSTRUCTION("_add_sp_sp_imm_16 sp,sp,#%d\n", imm7);
}

void _sub_sp_sp_imm_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t imm7 = LOW_BIT16(ins_code, 7) << 2;
    uint32_t Rd = 13;
    uint32_t setflags = FALSE;

    _sub_sp_imm(imm7, Rd, setflags, regs);
    LOG_INSTRUCTION("_sub_sp_sp_imm_16 sp,sp,#%d\n", imm7);
}

void _cbnz_cbz_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rn = LOW_BIT16(ins_code, 3);

    uint32_t i = LOW_BIT16(ins_code >> 9, 1);
    uint32_t imm5 = LOW_BIT16(ins_code>>3, 5);
    uint32_t imm = (imm5 << 1) | (i << 6);

    uint32_t nonzero = LOW_BIT16(ins_code>>11, 1);
    if(InITBlock(regs)){
        LOG_INSTRUCTION("UNPREDICTABLE: _cbnz_cbz_16 is treated as NOP\n");
        return;
    }

    _cbnz_cbz(imm, Rn, nonzero, regs);
    if(nonzero){
        LOG_INSTRUCTION("_cbnz_16 R%d,#%d\n", Rn, imm);
    }else{
        LOG_INSTRUCTION("_cbz_16 R%d,#%d\n", Rn, imm);
    }

}

inline void decode_rm_rd_16(uint16_t ins_code, uint32_t *Rm, uint32_t *Rd)
{
    *Rd = LOW_BIT16(ins_code, 3);
    *Rm = LOW_BIT16(ins_code>>3, 3);
}

void _sxth_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rd, Rm;
    decode_rm_rd_16(ins_code, &Rm, &Rd);
    uint32_t rotation = 0;

    _sxth(Rm, Rd, rotation, regs);
    LOG_INSTRUCTION("_sxth_16 R%d,R%d\n", Rd, Rm);
}

void _sxtb_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rd, Rm;
    decode_rm_rd_16(ins_code, &Rm, &Rd);
    uint32_t rotation = 0;

    _sxtb(Rm, Rd, rotation, regs);
    LOG_INSTRUCTION("_sxtb_16 R%d,R%d\n", Rd, Rm);
}

void _uxth_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rd, Rm;
    decode_rm_rd_16(ins_code, &Rm, &Rd);
    uint32_t rotation = 0;

    _uxth(Rm, Rd, rotation, regs);
    LOG_INSTRUCTION("_uxth_16 R%d,R%d\n", Rd, Rm);
}

void _uxtb_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rd, Rm;
    decode_rm_rd_16(ins_code, &Rm, &Rd);
    uint32_t rotation = 0;

    _uxtb(Rm, Rd, rotation, regs);
    LOG_INSTRUCTION("_uxtb_16 R%d,R%d\n", Rd, Rm);
}

void _push_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t register_list = LOW_BIT16(ins_code, 8);
    uint32_t registers = LOW_BIT16(ins_code >> 8, 1) << 14 | register_list;
    uint32_t bitcount = BitCount32(registers);
    if(bitcount < 1){
        LOG_INSTRUCTION("UNPREDICTABLE: push_16 is treated as NOP\n");
        return;
    }
    _push(registers, bitcount, cpu);
    LOG_INSTRUCTION("_push_16, %d\n", register_list);
}

void _rev_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rm, Rd;
    decode_rm_rd_16(ins_code, &Rm, &Rd);

    _rev(Rm, Rd, regs);
    LOG_INSTRUCTION("_rev_16, R%d,R%d\n", Rd, Rm);
}

void _rev16_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rm, Rd;
    decode_rm_rd_16(ins_code, &Rm, &Rd);

    _rev16(Rm, Rd, regs);
    LOG_INSTRUCTION("_rev16_16, R%d,R%d\n", Rd, Rm);
}

void _revsh_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t Rm, Rd;
    decode_rm_rd_16(ins_code, &Rm, &Rd);

    _revsh(Rm, Rd, regs);
    LOG_INSTRUCTION("revsh_16, R%d,R%d\n", Rd, Rm);
}

void _pop_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    uint32_t register_list = LOW_BIT16(ins_code, 8);
    uint32_t registers = LOW_BIT16(ins_code >> 8, 1) << 15 | register_list;
    uint32_t bitcount = BitCount32(registers);

    if(bitcount < 1){
        LOG_INSTRUCTION("UNPREDICTABLE: _pop_16 treated as NOP\n");
        return;
    }

    if((registers & (1ul<<15)) && InITBlock(regs) && !LastInITBlock(regs) ){
        LOG_INSTRUCTION("UNPREDICTABLE: _pop_16 treated as NOP\n");
        return;
    }

    _pop(registers, bitcount, cpu);
    LOG_INSTRUCTION("_pop_16, %d\n", register_list);
}

void _bkpt_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t imm32 = LOW_BIT16(ins_code, 8);

    LOG_INSTRUCTION("_bkpt_16 #%d\n", imm32);
}

void _it_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    thumb_state* state = (thumb_state*)cpu->run_info.cpu_spec_info;
    uint32_t mask = LOW_BIT16(ins_code, 4);
    uint32_t firstcond = LOW_BIT16(ins_code>>4, 4);
    if(firstcond == 0xF || (firstcond == 0xE && BitCount32(mask) != 1)){
        LOG_INSTRUCTION("UNPREDICTABLE: _it_16 treated as NOP\n");
        return;
    }
    if(InITBlock(regs)){
        LOG_INSTRUCTION("UNPREDICTABLE: _it_16 treated as NOP\n");
        return;
    }

    _it(firstcond, mask, regs, state);
    LOG_INSTRUCTION("_it_16, #%d\n", firstcond);
}

void _nop_16(uint16_t ins_code, cpu_t* cpu)
{
    // nop: do nothing
}

/* This is a sub-function with type of THUMB_DECODER */
thumb_translate16_t _it_hint_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t opB = LOW_BIT16(ins_code, 4);
    uint32_t opA = LOW_BIT16(ins_code>>4, 4);

    if(opB != 0){
        return (thumb_translate16_t)_it_16;
    }else{
        switch(opA){
        case 0x0:
            /* NOP: do nothing */
            LOG_INSTRUCTION("NOP\n");
            return (thumb_translate16_t)_nop_16;
            break;
        case 0x1:
            /* YIELD: TODO: */
            LOG_INSTRUCTION("YIELD treated as NOP\n");
            return (thumb_translate16_t)_nop_16;
            break;
        case 0x2:
            /* WFE: wait for event */
            LOG_INSTRUCTION("WFE treated as NOP\n");
            return (thumb_translate16_t)_nop_16;
            break;
        case 0x3:
            /* WFI: wait for interrupt */
            LOG_INSTRUCTION("WFI treated as NOP\n");
            return (thumb_translate16_t)_nop_16;
            break;
        case 0x4:
            /* SEV: send event hint */
            LOG_INSTRUCTION("SEV treated as NOP\n");
            return (thumb_translate16_t)_nop_16;
            break;
        default:
            return NULL;
            break;
        }
    }
}

void _stm_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t registers = LOW_BIT16(ins_code, 8);
    uint32_t Rn = LOW_BIT16(ins_code>>8, 3);
    bool_t wback = TRUE;
    uint32_t bitcount = BitCount32(registers);
    if(bitcount < 1){
        LOG_INSTRUCTION("UNPREDICTABLE: _stm_16 treated as NOP\n");
        return;
    }

    _stm(Rn, registers, bitcount, wback, cpu);
    LOG_INSTRUCTION("_stm_16, R#%d!,%d\n", Rn, registers);
}

void _ldm_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t registers = LOW_BIT16(ins_code, 8);
    uint32_t Rn = LOW_BIT16(ins_code>>8, 3);
    bool_t wback = FALSE;
    if((registers & (1ul << Rn)) == 0){
        wback = TRUE;
    }
    uint32_t bitcount = BitCount32(registers);
    if(bitcount < 1){
        LOG_INSTRUCTION("UNPREDICTABLE: _ldm_16 treated as NOP\n");
        return;
    }

    _ldm(Rn, registers, bitcount, wback, cpu);
    LOG_INSTRUCTION("_ldm_16, R#%d!,%d\n", Rn, registers);
}

void _con_b_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    uint8_t imm8 = LOW_BIT16(ins_code, 8);
    uint8_t cond = LOW_BIT16(ins_code >> 8, 4);

    int32_t imm32 = (int8_t)imm8 << 1;
    if(InITBlock(regs)){
        LOG_INSTRUCTION("UNPREDICTABLE: _con_b_16 treated as NOP\n");
        return;
    }

    _b(imm32, cond, cpu);
    LOG_INSTRUCTION("_con_b_16, 0x%x\n", imm32);
}

void _svc_16(uint16_t ins_code, cpu_t* cpu)
{
    uint32_t imm32 = LOW_BIT16(ins_code, 8);

    cpu->cm_NVIC->throw_exception(11, cpu->cm_NVIC);

    LOG_INSTRUCTION("_svc_16, #%d\n", imm32);
}

void _uncon_b_16(uint16_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    uint8_t imm8 = LOW_BIT16(ins_code>>3, 8);
    int32_t imm32 = (int8_t)imm8 << 4 | LOW_BIT16(ins_code, 3) << 1;
    if(InITBlock(regs)){
        LOG_INSTRUCTION("UNPREDICTABLE: _uncon_b_16 treated as NOP\n");
        return;
    }

    _b(imm32, 0, cpu);
    LOG_INSTRUCTION("_uncon_b_16, 0x%x\n", imm32);
}

void _unpredictable_16(uint16_t ins_code, cpu_t* cpu)
{
    LOG(LOG_WARN, "ins_code: %x is UNPREDICTABLE, treated as NOP\n", ins_code);
}

void _unpredictable_32(uint32_t ins_code, cpu_t *cpu)
{
    LOG(LOG_WARN, "ins_code: %x is UNPREDICTABLE, treated as NOP\n", ins_code);
}

static inline void stm_rn_registers_wback(uint32_t ins_code, uint32_t *Rn, uint32_t *registers, bool_t *wback)
{
    *Rn = LOW_BIT32(ins_code >> 16, 4);
    *registers = LOW_BIT32(ins_code, 16) & ~(0x5ul << 13);
    *wback = LOW_BIT32(ins_code >> 21, 1);
}

void _stm_32(uint32_t ins_code, cpu_t* cpu)
{
    uint32_t Rn, registers;
    bool_t wback;
    stm_rn_registers_wback(ins_code, &Rn, &registers, &wback);
    uint32_t bitcout = BitCount32(registers);

    if(Rn == 15 || bitcout < 2){
        LOG_INSTRUCTION("UNPREDICTABLE: _stm_32 treated as NOP\n");
        return;
    }
    if(wback && (registers & 1ul << Rn)){
        LOG_INSTRUCTION("UNPREDICTABLE: _stm_32 treated as NOP\n");
        return;

    }
    _stm(Rn, registers, bitcout, wback, cpu);
    LOG_INSTRUCTION("_stm_32, R%d!,%d\n", Rn, registers);
}

static inline void ldm_rn_register_wback(uint32_t ins_code, uint32_t *Rn, uint32_t *registers, bool_t *wback)
{
    *Rn = LOW_BIT32(ins_code >> 16, 4);
    *registers = LOW_BIT32(ins_code, 16) & ~(0x1ul << 13);
    *wback = LOW_BIT32(ins_code >> 21, 1);
}

void _ldm_32(uint32_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    uint32_t Rn, registers;
    bool_t wback;
    ldm_rn_register_wback(ins_code, &Rn, &registers, &wback);
    uint32_t bitcout = BitCount32(registers);

    if(Rn == 15 || bitcout < 2 || (registers >> 14) == 0x3){
        LOG_INSTRUCTION("UNPREDICTABLE: _ldm_32 treated as NOP\n");
    }
    if((registers & (1ul << 15)) && InITBlock(regs) && !LastInITBlock(regs)){
        LOG_INSTRUCTION("UNPREDICTABLE: _ldm_32 treated as NOP\n");
    }
    if(wback && (registers & (1ul << Rn))){
        LOG_INSTRUCTION("UNPREDICTABLE: _ldm_32 treated as NOP\n");
    }

    _ldm(Rn, registers, bitcout, wback, cpu);
    LOG_INSTRUCTION("_ldm_32, R%d,%d\n", Rn, registers);
}

void _pop_32(uint32_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    uint32_t Rn, registers;
    bool_t wback;
    ldm_rn_register_wback(ins_code, &Rn, &registers, &wback);
    uint32_t bitcount = BitCount32(registers);

    if(bitcount < 2 || (registers >> 14) == 0x3){
        LOG_INSTRUCTION("UNPREDICTABLE: _pop_32 treated as NOP\n");
    }
    if((registers & (1ul << 15)) && InITBlock(regs) && !LastInITBlock(regs)){
        LOG_INSTRUCTION("UNPREDICTABLE: _pop_32 treated as NOP\n");
    }

    _pop(registers, bitcount, cpu);
    LOG_INSTRUCTION("_pop_32, %d\n", registers);

}

thumb_translate32_t ldm_pop_bundle(uint32_t ins_code, cpu_t *cpu)
{
    uint32_t WRn = LOW_BIT32(ins_code >> 16, 4) | LOW_BIT32(ins_code >> 21, 1) << 4;
    if(WRn == 0x1D){
        return (thumb_translate32_t)_pop_32;
    }else{
        return (thumb_translate32_t)_ldm_32;
    }
}

void _push_32(uint32_t ins_code, cpu_t* cpu)
{
    uint32_t Rn, registers;
    bool_t wback;
    stm_rn_registers_wback(ins_code, &Rn, &registers, &wback);
    uint32_t bitcount = BitCount32(registers);
    if(bitcount < 2){
        LOG_INSTRUCTION("UNPREDICTABLE: _pop_32 treated as NOP\n");
    }

    _push(registers, bitcount, cpu);
    LOG_INSTRUCTION("_push_32, %d\n", registers);
}


void _stmdb_32(uint32_t ins_code, cpu_t *cpu)
{
    uint32_t Rn, registers;
    bool_t wback;
    stm_rn_registers_wback(ins_code, &Rn, &registers, &wback);
    uint32_t bitcount = BitCount32(registers);

    if(Rn == 15 || bitcount < 2){
        LOG_INSTRUCTION("UNPREDICTABLE: _stmdb_32 treated as NOP\n");
    }
    if(wback && (registers & (1ul << Rn))){
        LOG_INSTRUCTION("UNPREDICTABLE: _stmdb_32 treated as NOP\n");
    }

    _stmdb(Rn, registers, bitcount, wback, cpu);
    LOG_INSTRUCTION("_stmdb_32, R%d,%d\n", Rn, registers);
}

thumb_translate32_t stmdb_push_bundle(uint32_t ins_code, cpu_t *cpu)
{
    uint32_t WRn = LOW_BIT32(ins_code >> 16, 4) | LOW_BIT32(ins_code >> 21, 1) << 4;
    if(WRn == 0x1D){
        return (thumb_translate32_t)_push_32;
    }else{
        return (thumb_translate32_t)_stmdb_32;
    }
}

void _ldmdb_32(uint32_t ins_code, cpu_t* cpu)
{
    arm_reg_t* regs = ARMv7m_GET_REGS(cpu);
    uint32_t Rn, registers;
    bool_t wback;
    ldm_rn_register_wback(ins_code, &Rn, &registers, &wback);
    uint32_t bitcout = BitCount32(registers);

    if(Rn == 15 || bitcout < 2 || (registers >> 14) == 0x3){
        LOG_INSTRUCTION("UNPREDICTABLE: _ldmdb_32 treated as NOP\n");
    }
    if((registers & (1ul << 15)) && InITBlock(regs) && !LastInITBlock(regs)){
        LOG_INSTRUCTION("UNPREDICTABLE: _ldmdb_32 treated as NOP\n");
    }
    if(wback && (registers & (1ul << Rn))){
        LOG_INSTRUCTION("UNPREDICTABLE: _ldmdb_32 treated as NOP\n");
    }

    _ldmdb(Rn, registers, bitcout, wback, cpu);
    LOG_INSTRUCTION("_ldmdb_32, R%d,%d\n", Rn, registers);
}

void _strex_32(uint32_t ins_code, cpu_t* cpu)
{
    uint32_t imm8 = LOW_BIT32(ins_code, 8);
    uint32_t imm32 = imm8 << 2;
    uint32_t Rd = LOW_BIT32(ins_code >> 8, 4);
    uint32_t Rt = LOW_BIT32(ins_code >> 12, 4);
    uint32_t Rn = LOW_BIT32(ins_code >> 16, 4);

    if(IN_RANGE(Rd, 13, 15) || IN_RANGE(Rt, 13, 15) || Rn == 15){
        LOG_INSTRUCTION("UNPREDICTABLE: _strex_32 treated as NOP\n");
    }

    if(Rd == Rn || Rd == Rt){
        LOG_INSTRUCTION("UNPREDICTABLE: _strex_32 treated as NOP\n");
    }

    _strex(imm32, Rn, Rd, Rt, cpu);
    LOG_INSTRUCTION("_strex_32, R%d,R%d,[R%d,#%d]\n", Rd, Rt, Rn, imm8);
}

void _ldrex_32(uint32_t ins_code, cpu_t* cpu)
{
    uint32_t imm8 = LOW_BIT32(ins_code, 8);
    uint32_t imm32 = imm8 << 2;
    uint32_t Rt = LOW_BIT32(ins_code >> 12, 4);
    uint32_t Rn = LOW_BIT32(ins_code >> 16, 4);

    if(IN_RANGE(Rt, 13, 15) || Rn == 15){
        LOG_INSTRUCTION("UNPREDICTABLE: _ldrex_32 treated as NOP\n");
    }
    _ldrex(imm32, Rn, Rt, cpu);
    LOG_INSTRUCTION("_ldrex_32, R%d,[R%d,#%d]\n", Rt, Rn, imm8);
}

void _strd_32(uint32_t ins_code, cpu_t* cpu)
{
    uint32_t imm8 = LOW_BIT32(ins_code, 8);
    uint32_t imm32 = imm8 << 2;
    uint32_t Rt2 = LOW_BIT32(ins_code >> 8, 4);
    uint32_t Rt = LOW_BIT32(ins_code >> 12, 4);
    uint32_t Rn = LOW_BIT32(ins_code >> 16, 4);
    uint8_t W = get_bit(&ins_code, 21);
    uint8_t U = get_bit(&ins_code, 23);
    uint8_t P = get_bit(&ins_code, 24);

    int index = P == 0;
    int add = U == 1;
    int wback = W == 1;

    if(wback && (Rn == Rt || Rn == Rt2)){
        LOG_INSTRUCTION("UNPREDICTABLE: _strd_32 treated as NOP\n");
    }
    if(Rn == 15 || IN_RANGE(Rt, 13, 15) || IN_RANGE(Rt2, 13, 15)){
        LOG_INSTRUCTION("UNPREDICTABLE: _strd_32 treated as NOP\n");
    }

    _strd(imm32, Rn, Rt, Rt2, add, wback, index, cpu);
    LOG_INSTRUCTION("_strd_32, R%d,R%d,[R%d,%c#%d]\n", Rt, Rt2, Rn, add?'+':'-', imm8);
}

void _ldrd_imm_32(uint32_t ins_code, cpu_t* cpu)
{
    uint32_t imm8 = LOW_BIT32(ins_code, 8);
    uint32_t imm32 = imm8 << 2;
    uint32_t Rt2 = LOW_BIT32(ins_code >> 8, 4);
    uint32_t Rt = LOW_BIT32(ins_code >> 12, 4);
    uint32_t Rn = LOW_BIT32(ins_code >> 16, 4);
    uint8_t W = get_bit(&ins_code, 21);
    uint8_t U = get_bit(&ins_code, 23);
    uint8_t P = get_bit(&ins_code, 24);

    int index = P == 0;
    int add = U == 1;
    int wback = W == 1;

    if(wback && (Rn == Rt || Rn == Rt2)){
        LOG_INSTRUCTION("UNPREDICTABLE: _ldrd_imm_32 treated as NOP\n");
    }
    if(Rn == 15 || IN_RANGE(Rt, 13, 15) || IN_RANGE(Rt2, 13, 15) || Rt == Rt2){
        LOG_INSTRUCTION("UNPREDICTABLE: _ldrd_imm_32 treated as NOP\n");
    }

    _ldrd_imm(imm32, Rn, Rt, Rt2, add, wback, index, cpu);
    LOG_INSTRUCTION("_ldrd_imm_32, R%d,R%d,[R%d,%c#%d]\n", Rt, Rt2, Rn, add?'+':'-', imm8);
}

void _ldrd_literal_32(uint32_t ins_code, cpu_t *cpu)
{
    uint32_t imm8 = LOW_BIT32(ins_code, 8);
    uint32_t imm32 = imm8 << 2;
    uint32_t Rt2 = LOW_BIT32(ins_code >> 8, 4);
    uint32_t Rt = LOW_BIT32(ins_code >> 12, 4);
    uint8_t W = get_bit(&ins_code, 21);
    uint8_t U = get_bit(&ins_code, 23);
    int add = U == 1;

    if(W || IN_RANGE(Rt, 13, 15) || IN_RANGE(Rt2, 13, 15) || Rt == Rt2){
        LOG_INSTRUCTION("UNPREDICTABLE: _ldrd_literal_32 treated as NOP\n");
    }

    _ldrd_literal(imm32, Rt, Rt2, add, cpu);
    LOG_INSTRUCTION("_ldrd_literal_32, R%d,R%d,[PC,%c#%d]\n", Rt, Rt2, add?'+':'-', imm8);
}

thumb_translate32_t _ldrd_32(uint32_t ins_code, cpu_t* cpu)
{
    uint32_t Rn = LOW_BIT32(ins_code >> 16, 4);
    if(Rn == 0xF){
        return (thumb_translate32_t)_ldrd_literal_32;
    }else{
        return (thumb_translate32_t)_ldrd_imm_32;
    }
}

void _strexb_32(uint32_t ins_code, cpu_t *cpu)
{
    uint32_t Rd = LOW_BIT32(ins_code, 4);
    uint32_t Rt = LOW_BIT32(ins_code >> 12, 4);
    uint32_t Rn = LOW_BIT32(ins_code >> 16, 4);

    if(IN_RANGE(Rd, 13, 15) || IN_RANGE(Rt, 13, 15) || Rn == 15){
        LOG_INSTRUCTION("UNPREDICTABLE: _strexb_32 treated as NOP\n");
    }

    if(Rd == Rn || Rd == Rt){
        LOG_INSTRUCTION("UNPREDICTABLE: _strexb_32 treated as NOP\n");
    }

    _strexb(Rd, Rt, Rn, cpu);
    LOG_INSTRUCTION("_strexb_32, R%d,R%d,[R%d]\n", Rd, Rt, Rn);
}

void _strexh_32(uint32_t ins_code, cpu_t *cpu)
{
    uint32_t Rd = LOW_BIT32(ins_code, 4);
    uint32_t Rt = LOW_BIT32(ins_code >> 12, 4);
    uint32_t Rn = LOW_BIT32(ins_code >> 16, 4);

    if(IN_RANGE(Rd, 13, 15) || IN_RANGE(Rt, 13, 15) || Rn == 15){
        LOG_INSTRUCTION("UNPREDICTABLE: _strexh_32 treated as NOP\n");
    }

    if(Rd == Rn || Rd == Rt){
        LOG_INSTRUCTION("UNPREDICTABLE: _strexh_32 treated as NOP\n");
    }

    _strexh(Rd, Rt, Rn, cpu);
    LOG_INSTRUCTION("_strexh_32, R%d,R%d,[R%d]\n", Rd, Rt, Rn);
}

thumb_translate32_t _strexb_h_32(uint32_t ins_code, cpu_t *cpu)
{
    uint32_t op3 = LOW_BIT32(ins_code >> 4, 4);
    if(op3 == 0x4){
        return (thumb_translate32_t)_strexb_32;
    }else if(op3 == 0x5){
        return (thumb_translate32_t)_strexh_32;
    }else{
        return (thumb_translate32_t)_unpredictable_32;
    }
}

void _tbb_h_32(uint32_t ins_code, cpu_t *cpu)
{
    uint32_t Rm = LOW_BIT32(ins_code, 4);
    uint32_t Rn = LOW_BIT32(ins_code >> 16, 4);
    bool_t is_tbh = LOW_BIT32(ins_code >> 4, 1);

    if(Rn == 13 || IN_RANGE(Rm, 13, 15)){
        LOG_INSTRUCTION("UNPREDICTABLE: _tbb_h_32 treated as NOP\n");
    }

    if(InITBlock(cpu->regs) && !LastInITBlock(cpu->regs)){
        LOG_INSTRUCTION("UNPREDICTABLE: _tbb_h_32 treated as NOP\n");
    }

    _tbb_h(Rn, Rm, is_tbh, cpu);
    if(!is_tbh){
        LOG_INSTRUCTION("_tbb_32, [R%d,R%d]\n", Rn, Rm);
    }else{
        LOG_INSTRUCTION("_tbh_32, [R%d,R%d,LSL #1]\n", Rn, Rm);
    }
}

void _ldrexb_32(uint32_t ins_code, cpu_t *cpu)
{
    uint32_t Rt = LOW_BIT32(ins_code >> 12, 4);
    uint32_t Rn = LOW_BIT32(ins_code >> 16, 4);

    if(IN_RANGE(Rt, 13, 15) || Rn == 15){
        LOG_INSTRUCTION("UNPREDICTABLE: _ldrexb_32 treated as NOP\n");
    }

    _ldrexb(Rn, Rt, cpu);
    LOG_INSTRUCTION("_ldrexb_32, R%d,[R%d]\n", Rt, Rn);
}

void _ldrexh_32(uint32_t ins_code, cpu_t *cpu)
{
    uint32_t Rt = LOW_BIT32(ins_code >> 12, 4);
    uint32_t Rn = LOW_BIT32(ins_code >> 16, 4);

    if(IN_RANGE(Rt, 13, 15) || Rn == 15){
        LOG_INSTRUCTION("UNPREDICTABLE: _ldrexh_32 treated as NOP\n");
    }

    _ldrexh(Rn, Rt, cpu);
    LOG_INSTRUCTION("_ldrexh_32, R%d,[R%d]\n", Rt, Rn);
}

thumb_translate32_t _tbb_h_ldrexb_h_32(uint32_t ins_code, cpu_t *cpu)
{
    uint32_t op3 = LOW_BIT32(ins_code >> 4, 4);
    switch(op3){
    case 0:
    case 1:
        return (thumb_translate32_t)_tbb_h_32;
    case 0x4:
        return (thumb_translate32_t)_ldrexb_32;
    case 0x5:
        return (thumb_translate32_t)_ldrexh_32;
    default:
        return (thumb_translate32_t)_unpredictable_32;
    }
}

/****** init instruction table ******/
void init_instruction_table(thumb_instruct_table_t* table)
{
    LOG(LOG_DEBUG, "ARM size of instruction table is %d byte\n", sizeof(thumb_instruct_table_t));

    /*************************************** 16 bit thumb instructions *******************************************************/
    // 15~10 bit A5-156
    set_base_table_value(table->base_table16, 0x00, 0x0F, (thumb_translate_t)shift_add_sub_mov,     THUMB_DECODER);        // 00xxxx
    set_base_table_value(table->base_table16, 0x10, 0x10, (thumb_translate_t)data_process,          THUMB_DECODER);        // 010000
    set_base_table_value(table->base_table16, 0x11, 0x11, (thumb_translate_t)spdata_branch_exchange,THUMB_DECODER);        // 010001
    // load from literal pool A7-289
    set_base_table_value(table->base_table16, 0x12, 0x13, (thumb_translate_t)_ldr_literal_16,       THUMB_EXCUTER);        // 01001x
    set_base_table_value(table->base_table16, 0x14, 0x27, (thumb_translate_t)load_store_single,     THUMB_DECODER);        // 0101xx 011xxx 100xxx
    // pc related address A7-229
    set_base_table_value(table->base_table16, 0x28, 0x29, (thumb_translate_t)_adr_16,               THUMB_EXCUTER);        // 10100x
    // sp related address A7-225
    set_base_table_value(table->base_table16, 0x2A, 0x2B, (thumb_translate_t)_add_sp_imm_16,        THUMB_EXCUTER);        // 10101x
    set_base_table_value(table->base_table16, 0x2C, 0x2F, (thumb_translate_t)misc_16bit_ins,        THUMB_DECODER);        // 1011xx
    set_base_table_value(table->base_table16, 0x30, 0x31, (thumb_translate_t)_stm_16,               THUMB_EXCUTER);        // 11000x
    set_base_table_value(table->base_table16, 0x32, 0x33, (thumb_translate_t)_ldm_16,               THUMB_EXCUTER);        // 11001x
    set_base_table_value(table->base_table16, 0x34, 0x37, (thumb_translate_t)con_branch_svc,        THUMB_DECODER);        // 1101xx
    set_base_table_value(table->base_table16, 0x38, 0x39, (thumb_translate_t)_uncon_b_16,           THUMB_EXCUTER);        // 11100x

    // shift(imm) add sub mov A5-157
    set_sub_table_value(table->shift_add_sub_mov_table16, 0x00, 0x03, (thumb_translate_t)_lsl_imm_16, THUMB_EXCUTER);    // 000xx
    set_sub_table_value(table->shift_add_sub_mov_table16, 0x04, 0x07, (thumb_translate_t)_lsr_imm_16, THUMB_EXCUTER);    // 001xx
    set_sub_table_value(table->shift_add_sub_mov_table16, 0x08, 0x0B, (thumb_translate_t)_asr_imm_16, THUMB_EXCUTER);    // 010xx
    set_sub_table_value(table->shift_add_sub_mov_table16, 0x0C, 0x0C, (thumb_translate_t)_add_reg_16, THUMB_EXCUTER);    // 01100
    set_sub_table_value(table->shift_add_sub_mov_table16, 0x0D, 0x0D, (thumb_translate_t)_sub_reg_16, THUMB_EXCUTER);    // 01101
    set_sub_table_value(table->shift_add_sub_mov_table16, 0x0E, 0x0E, (thumb_translate_t)_add_imm3_16, THUMB_EXCUTER);    // 01110
    set_sub_table_value(table->shift_add_sub_mov_table16, 0x0F, 0x0F, (thumb_translate_t)_sub_imm3_16, THUMB_EXCUTER);    // 01111
    set_sub_table_value(table->shift_add_sub_mov_table16, 0x10, 0x13, (thumb_translate_t)_mov_imm_16, THUMB_EXCUTER);    // 100xx
    set_sub_table_value(table->shift_add_sub_mov_table16, 0x14, 0x17, (thumb_translate_t)_cmp_imm_16, THUMB_EXCUTER);    // 101xx
    set_sub_table_value(table->shift_add_sub_mov_table16, 0x18, 0x1B, (thumb_translate_t)_add_imm8_16, THUMB_EXCUTER);    // 110xx
    set_sub_table_value(table->shift_add_sub_mov_table16, 0x1C, 0x1F, (thumb_translate_t)_sub_imm8_16, THUMB_EXCUTER);    // 111xx

    // data processing A5-158
    set_sub_table_value(table->data_process_table16, 0x00, 0x00, (thumb_translate_t)_and_reg_16, THUMB_EXCUTER);        // 0000
    set_sub_table_value(table->data_process_table16, 0x01, 0x01, (thumb_translate_t)_eor_reg_16, THUMB_EXCUTER);        // 0001
    set_sub_table_value(table->data_process_table16, 0x02, 0x02, (thumb_translate_t)_lsl_reg_16, THUMB_EXCUTER);        // 0010
    set_sub_table_value(table->data_process_table16, 0x03, 0x03, (thumb_translate_t)_lsr_reg_16, THUMB_EXCUTER);        // 0011
    set_sub_table_value(table->data_process_table16, 0x04, 0x04, (thumb_translate_t)_asr_reg_16, THUMB_EXCUTER);        // 0100
    set_sub_table_value(table->data_process_table16, 0x05, 0x05, (thumb_translate_t)_adc_reg_16, THUMB_EXCUTER);        // 0101
    set_sub_table_value(table->data_process_table16, 0x06, 0x06, (thumb_translate_t)_sbc_reg_16, THUMB_EXCUTER);        // 0110
    set_sub_table_value(table->data_process_table16, 0x07, 0x07, (thumb_translate_t)_ror_reg_16, THUMB_EXCUTER);        // 0111
    set_sub_table_value(table->data_process_table16, 0x08, 0x08, (thumb_translate_t)_tst_reg_16, THUMB_EXCUTER);        // 1000
    set_sub_table_value(table->data_process_table16, 0x09, 0x09, (thumb_translate_t)_rsb_imm_16, THUMB_EXCUTER);        // 1001
    set_sub_table_value(table->data_process_table16, 0x0A, 0x0A, (thumb_translate_t)_cmp_reg_16, THUMB_EXCUTER);        // 1010
    set_sub_table_value(table->data_process_table16, 0x0B, 0x0B, (thumb_translate_t)_cmn_reg_16, THUMB_EXCUTER);        // 1011
    set_sub_table_value(table->data_process_table16, 0x0C, 0x0C, (thumb_translate_t)_orr_reg_16, THUMB_EXCUTER);        // 1100
    set_sub_table_value(table->data_process_table16, 0x0D, 0x0D, (thumb_translate_t)_mul_reg_16, THUMB_EXCUTER);        // 1101
    set_sub_table_value(table->data_process_table16, 0x0E, 0x0E, (thumb_translate_t)_bic_reg_16, THUMB_EXCUTER);        // 1110
    set_sub_table_value(table->data_process_table16, 0x0F, 0x0F, (thumb_translate_t)_mvn_reg_16, THUMB_EXCUTER);        // 1111

    // special data instructions and branch and exchange A5-159
    set_sub_table_value(table->spdata_branch_exchange_table16, 0x00, 0x03, (thumb_translate_t)_add_reg_spec_16, THUMB_EXCUTER);    // 00xx
    set_sub_table_value(table->spdata_branch_exchange_table16, 0x04, 0x04, (thumb_translate_t)_unpredictable_16,THUMB_EXCUTER);    // 0100(*)
    set_sub_table_value(table->spdata_branch_exchange_table16, 0x05, 0x07, (thumb_translate_t)_cmp_reg_spec_16, THUMB_EXCUTER);    // 0101,011x
    set_sub_table_value(table->spdata_branch_exchange_table16, 0x08, 0x0B, (thumb_translate_t)_mov_reg_spec_16, THUMB_EXCUTER);    // 10xx
    set_sub_table_value(table->spdata_branch_exchange_table16, 0x0C, 0x0D, (thumb_translate_t)_bx_spec_16,      THUMB_EXCUTER);    // 110x
    set_sub_table_value(table->spdata_branch_exchange_table16, 0x0E, 0x0F, (thumb_translate_t)_blx_spec_16,     THUMB_EXCUTER);    // 111x

    // load store single data item A5-160
    set_sub_table_value(table->load_store_single_table16, 0x00, 0x27, (thumb_translate_t)_unpredictable_16, THUMB_EXCUTER);        // 0000 000 ~ 0010 011
    set_sub_table_value(table->load_store_single_table16, 0x28, 0x28, (thumb_translate_t)_str_reg_16,       THUMB_EXCUTER);        // 0101 000
    set_sub_table_value(table->load_store_single_table16, 0x29, 0x29, (thumb_translate_t)_strh_reg_16,      THUMB_EXCUTER);        // 0101 001
    set_sub_table_value(table->load_store_single_table16, 0x2A, 0x2A, (thumb_translate_t)_strb_reg_16,      THUMB_EXCUTER);        // 0101 010
    set_sub_table_value(table->load_store_single_table16, 0x2B, 0x2B, (thumb_translate_t)_ldrsb_reg_16,     THUMB_EXCUTER);        // 0101 011
    set_sub_table_value(table->load_store_single_table16, 0x2C, 0x2C, (thumb_translate_t)_ldr_reg_16,       THUMB_EXCUTER);        // 0101    100
    set_sub_table_value(table->load_store_single_table16, 0x2D, 0x2D, (thumb_translate_t)_ldrh_reg_16,      THUMB_EXCUTER);        // 0101 101
    set_sub_table_value(table->load_store_single_table16, 0x2E, 0x2E, (thumb_translate_t)_ldrb_reg_16,      THUMB_EXCUTER);        // 0101 110
    set_sub_table_value(table->load_store_single_table16, 0x2F, 0x2F, (thumb_translate_t)_ldrsh_reg_16,     THUMB_EXCUTER);        // 0101 111
    set_sub_table_value(table->load_store_single_table16, 0x2F, 0x2F, (thumb_translate_t)_ldrsh_reg_16,     THUMB_EXCUTER);        // 0101 111
    set_sub_table_value(table->load_store_single_table16, 0x30, 0x33, (thumb_translate_t)_str_imm_16,       THUMB_EXCUTER);        // 0110 0xx
    set_sub_table_value(table->load_store_single_table16, 0x34, 0x37, (thumb_translate_t)_ldr_imm_16,       THUMB_EXCUTER);        // 0110 1xx
    set_sub_table_value(table->load_store_single_table16, 0x38, 0x3B, (thumb_translate_t)_strb_imm_16,      THUMB_EXCUTER);        // 0111 0xx
    set_sub_table_value(table->load_store_single_table16, 0x3C, 0x3F, (thumb_translate_t)_ldrb_imm_16,      THUMB_EXCUTER);        // 0111 1xx
    set_sub_table_value(table->load_store_single_table16, 0x40, 0x43, (thumb_translate_t)_strh_imm_16,      THUMB_EXCUTER);        // 1000 0xx
    set_sub_table_value(table->load_store_single_table16, 0x44, 0x47, (thumb_translate_t)_ldrh_imm_16,      THUMB_EXCUTER);        // 1000 1xx
    set_sub_table_value(table->load_store_single_table16, 0x48, 0x4B, (thumb_translate_t)_str_sp_imm_16,    THUMB_EXCUTER);        // 1001 0xx
    set_sub_table_value(table->load_store_single_table16, 0x4C, 0x4F, (thumb_translate_t)_ldr_sp_imm_16,    THUMB_EXCUTER);        // 1001 1xx

    // misc 16-bit instructions A5-161
    set_sub_table_value(table->misc_16bit_ins_table16, 0x00, MISC_16BIT_INS_SIZE-1, (thumb_translate_t)_unpredictable_16, THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x00, 0x03, (thumb_translate_t)_add_sp_sp_imm_16,    THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x04, 0x07, (thumb_translate_t)_sub_sp_sp_imm_16,    THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x08, 0x0F, (thumb_translate_t)_cbnz_cbz_16,         THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x10, 0x11, (thumb_translate_t)_sxth_16,             THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x12, 0x13, (thumb_translate_t)_sxtb_16,             THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x14, 0x15, (thumb_translate_t)_uxth_16,             THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x16, 0x17, (thumb_translate_t)_uxtb_16,             THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x18, 0x1F, (thumb_translate_t)_cbnz_cbz_16,         THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x20, 0x2F, (thumb_translate_t)_push_16,             THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x48, 0x4F, (thumb_translate_t)_cbnz_cbz_16,         THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x50, 0x51, (thumb_translate_t)_rev_16,              THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x52, 0x53, (thumb_translate_t)_rev16_16,            THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x56, 0x57, (thumb_translate_t)_revsh_16,            THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x58, 0x5F, (thumb_translate_t)_cbnz_cbz_16,         THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x60, 0x6F, (thumb_translate_t)_pop_16,              THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x70, 0x77, (thumb_translate_t)_bkpt_16,             THUMB_EXCUTER);
    set_sub_table_value(table->misc_16bit_ins_table16, 0x78, 0x7F, (thumb_translate_t)_it_hint_16,          THUMB_DECODER);

    // conditional branch and supervisor call
    set_sub_table_value(table->con_branch_svc_table16, 0x00, 0x0D, (thumb_translate_t)_con_b_16,        THUMB_EXCUTER);
    set_sub_table_value(table->con_branch_svc_table16, 0x0E, 0x0E, (thumb_translate_t)_unpredictable_16,THUMB_EXCUTER);
    set_sub_table_value(table->con_branch_svc_table16, 0x0F, 0x0F, (thumb_translate_t)_svc_16,          THUMB_EXCUTER);


    /*************************************** 32 bit thumb instructions *******************************************************/
    // bit 20~28
    // load multiple and store multiple A5-171
    set_base_table_value(table->main_table32, 0x088, 0x088, (thumb_translate_t)_stm_32,           THUMB_EXCUTER);
    set_base_table_value(table->main_table32, 0x08A, 0x08A, (thumb_translate_t)_stm_32,           THUMB_EXCUTER);
    set_base_table_value(table->main_table32, 0x089, 0x089, (thumb_translate_t)ldm_pop_bundle,    THUMB_DECODER);
    set_base_table_value(table->main_table32, 0x08B, 0x08B, (thumb_translate_t)ldm_pop_bundle,    THUMB_DECODER);
    set_base_table_value(table->main_table32, 0x090, 0x090, (thumb_translate_t)stmdb_push_bundle, THUMB_DECODER);
    set_base_table_value(table->main_table32, 0x092, 0x092, (thumb_translate_t)stmdb_push_bundle, THUMB_DECODER);
    set_base_table_value(table->main_table32, 0x091, 0x091, (thumb_translate_t)_ldmdb_32,         THUMB_EXCUTER);
    set_base_table_value(table->main_table32, 0x093, 0x093, (thumb_translate_t)_ldmdb_32,         THUMB_EXCUTER);

    // load store dual or exclusive, table brach A5-172
    set_base_table_value(table->main_table32, 0x084, 0x084, (thumb_translate_t)_strex_32, THUMB_EXCUTER);
    set_base_table_value(table->main_table32, 0x085, 0x085, (thumb_translate_t)_ldrex_32, THUMB_EXCUTER);

    set_base_table_value(table->main_table32, 0x086, 0x086, (thumb_translate_t)_strd_32, THUMB_EXCUTER);
    set_base_table_value(table->main_table32, 0x08E, 0x08E, (thumb_translate_t)_strd_32, THUMB_EXCUTER);
    set_base_table_value(table->main_table32, 0x094, 0x094, (thumb_translate_t)_strd_32, THUMB_EXCUTER);
    set_base_table_value(table->main_table32, 0x096, 0x096, (thumb_translate_t)_strd_32, THUMB_EXCUTER);
    set_base_table_value(table->main_table32, 0x09C, 0x09C, (thumb_translate_t)_strd_32, THUMB_EXCUTER);
    set_base_table_value(table->main_table32, 0x09E, 0x09E, (thumb_translate_t)_strd_32, THUMB_EXCUTER);

    set_base_table_value(table->main_table32, 0x087, 0x087, (thumb_translate_t)_ldrd_32, THUMB_DECODER);
    set_base_table_value(table->main_table32, 0x08F, 0x08F, (thumb_translate_t)_ldrd_32, THUMB_DECODER);
    set_base_table_value(table->main_table32, 0x095, 0x095, (thumb_translate_t)_ldrd_32, THUMB_DECODER);
    set_base_table_value(table->main_table32, 0x097, 0x097, (thumb_translate_t)_ldrd_32, THUMB_DECODER);
    set_base_table_value(table->main_table32, 0x09D, 0x09D, (thumb_translate_t)_ldrd_32, THUMB_DECODER);
    set_base_table_value(table->main_table32, 0x09F, 0x09F, (thumb_translate_t)_ldrd_32, THUMB_DECODER);

    set_base_table_value(table->main_table32, 0x08C, 0x08C, (thumb_translate_t)_strexb_h_32, THUMB_DECODER);
    set_base_table_value(table->main_table32, 0x08D, 0x08D, (thumb_translate_t)_tbb_h_ldrexb_h_32, THUMB_DECODER);
}

bool_t is_16bit_code(uint16_t opcode)
{
    uint16_t opcode_header = opcode >> 11;
    switch(opcode_header){
    case 0x1D:
    case 0x1E:
    case 0x1F:
        return FALSE;
    default:
        return TRUE;
    }
}

uint32_t align_address(uint32_t address)
{
    return address & 0xFFFFFFFE;
}

typedef struct{
    uint16_t opcode16_h;
    uint16_t opcode16_l;
}opcode_t;

/***** The main parsing function for the instruction *******/
/* The decode structure has 2 types: THUMB_DECODER and THUMB_EXCUTER.
   If the type is THUMB_DECODER, it means the sub-function is a decoder
   function which should be run and will return the excutable pointer to
   the opcode.
   If the type is THUMB_EXCUTER, it means the sub-function is already
   the excutable pointer to the opcode. In this case, just return the pointer.
   The same judgement should take place in all the sub-function with type
   of THUMB_DECODER. */
thumb_translate16_t thumb_parse_opcode16(uint16_t opcode, cpu_t* cpu)
{
    thumb_decode_t decode = M_translate_table->base_table16[opcode >> 10];
    if(decode.type == THUMB_DECODER){
        return (thumb_translate16_t)decode.translater16(opcode, cpu);
    }else{
        return decode.translater16;
    }
}

thumb_translate32_t thumb_parse_opcode32(uint32_t opcode, cpu_t *cpu)
{
    /* decode by bit 20 to 28 */
    thumb_decode_t decode = M_translate_table->main_table32[(opcode >> 20) & 0x1FF];
    if(decode.type == THUMB_DECODER){
        return (thumb_translate32_t)decode.translater32(opcode, cpu);
    }else{
        return decode.translater32;
    }
}

void armv7m_next_PC_16(arm_reg_t* regs)
{
    regs->PC += 2;
    regs->PC_return = regs->PC + 2;
}

void armv7m_next_PC_32(arm_reg_t* regs)
{
    regs->PC += 4;
    regs->PC_return = regs->PC;
}

void armv7m_next_PC(cpu_t* cpu, int ins_length)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    if(ins_length == 16){
        armv7m_next_PC_16(regs);
    }else if(ins_length == 32){
        armv7m_next_PC_32(regs);
    }
    /* store pc to indicate whether pc is changed by the opcode*/
    cpu->run_info.last_pc = regs->PC;
}

int armv7m_PC_modified(cpu_t* cpu)
{
    arm_reg_t* regs = (arm_reg_t*)cpu->regs;
    return (uint32_t)cpu->run_info.last_pc != regs->PC;
}

arm_reg_t* create_arm_regs()
{
    arm_reg_t* regs = (arm_reg_t*)calloc(1, sizeof(arm_reg_t));

    return regs;
}

error_code_t destory_arm_regs(arm_reg_t** regs)
{
    if(regs == NULL || *regs == NULL){
        return ERROR_NULL_POINTER;
    }

    free(*regs);
    *regs = NULL;

    return SUCCESS;
}

thumb_state* create_thumb_state()
{
    return (thumb_state*)malloc(sizeof(thumb_state))    ;
}

int destory_thumb_state(thumb_state** state)
{
    if(state == NULL || *state == NULL){
        return -ERROR_NULL_POINTER;
    }

    free(*state);
    *state = NULL;

    return SUCCESS;
}

thumb_global_state* create_thumb_global_state()
{
    thumb_global_state *gstate = (thumb_global_state*)calloc(1, sizeof(thumb_global_state));
    if(gstate == NULL){
        goto gstate_null;
    }

    gstate->local_exclusive = list_create_empty();
    if(gstate->local_exclusive == NULL){
        goto local_exclusive_null;
    }
    gstate->global_exclusive = list_create_empty();
    if(gstate->global_exclusive == NULL){
        goto gloabl_exclusive_null;
    }

    return gstate;

gloabl_exclusive_null:
    list_delete(gstate->local_exclusive);
local_exclusive_null:
    free(gstate);
gstate_null:
    return NULL;
}

int destory_thumb_global_state(thumb_global_state **state)
{
    if(state == NULL || *state == NULL){
        return -ERROR_NULL_POINTER;
    }
    free(*state);
    *state = NULL;

    return SUCCESS;
}

thumb_instruct_table_t* create_instruction_table()
{
    return (thumb_instruct_table_t*)malloc(sizeof(thumb_instruct_table_t));
}

void desotry_instruction_table(thumb_instruct_table_t **table)
{
    free(*table);
    table = NULL;
}

/* create and initialize the instruction as well as the cpu state */
int ins_thumb_init(_IO cpu_t* cpu)
{
    thumb_state* state = create_thumb_state();
    if(state == NULL){
        goto state_error;
    }
    set_cpu_spec_info(cpu, state);

    thumb_global_state *global_state = create_thumb_global_state();
    if(global_state == NULL){
        goto global_state_error;
    }
    cpu->run_info.global_info = global_state;

    cpu->regs = create_arm_regs();
    if(cpu->regs == NULL){
        goto regs_error;
    }

    /* translate table will init only once when needed */
    if(M_translate_table == NULL){
        M_translate_table = create_instruction_table();
    }
    if(M_translate_table == NULL){
        goto table_err;
    }
    init_instruction_table(M_translate_table);
    return SUCCESS;

table_err:
    destory_arm_regs((arm_reg_t**)&cpu->regs);
regs_error:
    destory_thumb_global_state((thumb_global_state**)&cpu->run_info.global_info);
global_state_error:
    destory_thumb_state((thumb_state**)&cpu->run_info.cpu_spec_info);
state_error:
    return -ERROR_CREATE;
}

int ins_thumb_destory(cpu_t* cpu)
{
    desotry_instruction_table(&M_translate_table);
    destory_arm_regs((arm_reg_t**)&cpu->regs);
    destory_thumb_state((thumb_state**)&cpu->run_info.cpu_spec_info);
    cpu->instruction_data = NULL;

    return SUCCESS;
}
