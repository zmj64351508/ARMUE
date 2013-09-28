#include "_types.h"
#include "arm_v7m_ins_decode.h"
#include "cpu.h"
#include <stdlib.h>
#include <assert.h>

armv7m_instruct_table_t translate_table;

uint32_t BitCount32(uint32_t bits)
{
	uint32_t count = 0;
	while(bits != 0){
		bits = bits & (bits-1);
		count++;
	}
	return count;
}

/******							IMPROTANT									*/
/****** PC always pointers to the address of next instruction.				*/
/****** when 16bit coded, PC += 2. when 32bit coded, PC += 4.				*/		
/****** But, when 16 bit coded instruction visit PC, it will return PC+2	*/


error_code_t set_base_table_value(armv7m_translate16_t* table, int start, int end, armv7m_translate16_t value)
{
	for(int i = start; i <= end; i++){
		table[i] = value;
	}

	return SUCCESS;
}
#define set_sub_table_value set_base_table_value

void print_reg_val(armv7m_reg_t* regs)
{
	for(int i = 0; i < 13; i++){
		printf("R%-3d=0x%08x\n", i, regs->R[i]);
	}

	printf("MSP =0x%08x\n", regs->MSP);
	printf("LR  =0x%08x\n", regs->LR);
	printf("PC  =0x%08x\n", regs->PC);
	printf("xPSR=0x%08x\n\n", regs->xPSR);
}

/****** Here are some sub-parsing functions *******/
armv7m_translate16_t shift_add_sub_mov(uint16_t ins_code, cpu_t* cpu)
{
	// call appropriate instruction implement function by inquiring 13~9bit
	return translate_table.shift_add_sub_mov_table16[ins_code >> 9 & 0x1F];
}

armv7m_translate16_t data_process(uint16_t ins_code, cpu_t* cpu)
{
	// call appropriate instruction implement function by inquiring 9~6bit
	return translate_table.data_process_table16[ins_code >> 6 & 0x0F];	
}

armv7m_translate16_t spdata_branch_exchange(uint16_t ins_code, cpu_t* cpu)
{
	// call appropriate instruction implement function by inquiring 9~6bit
	return translate_table.spdata_branch_exchange_table16[ins_code >> 6 & 0x0F];
}

armv7m_translate16_t load_literal_pool(uint16_t ins_code, cpu_t* cpu)
{
	/* load fron literal pool set has only 1 instruction */
	return translate_table.load_literal_table16[0];
}

armv7m_translate16_t load_store_single(uint16_t ins_code, cpu_t* cpu)
{
	// call appropriate instruction implement function by inquiring 15~9bit
	return translate_table.load_store_single_table16[ins_code >> 9 & 0x7F];
}

armv7m_translate16_t pc_related_address(uint16_t ins_code, cpu_t* cpu)
{
	return translate_table.pc_related_address_table16[0];
}

armv7m_translate16_t sp_related_address(uint16_t ins_code, cpu_t* cpu)
{
	return translate_table.sp_related_address_table16[0];
}

armv7m_translate16_t misc_16bit_ins(uint16_t ins_code, cpu_t* cpu)
{
	return translate_table.misc_16bit_ins_table[ins_code >> 5 & 0x7F];
}

/****** Here are the final decoders of 16bit instructions ******/
void _lsl_imm_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t imm = ins_code >> 6 & 0x1F;
	uint32_t Rm = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);

	_lsl_imm(imm, Rm, Rd, setflags, regs);
	LOG_INSTRUCTION("_lsl_imm_16, R%d,R%d,#%d\n", Rd, Rm, imm);
}

void _lsr_imm_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t imm = ins_code >> 6 & 0x1F;
	uint32_t Rm = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);
	
	_lsr_imm(imm, Rm, Rd, setflags, regs);		
	LOG_INSTRUCTION("_lsr_imm_16, R%d,R%d,#%d\n", Rd, Rm, imm);
}

void _asr_imm_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t imm = ins_code >> 6 & 0x1F;
	uint32_t Rm = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);

	_asr_imm(imm, Rm, Rd, setflags, regs);	
	LOG_INSTRUCTION("_asr_imm_16, R%d,R%d,#%d\n", Rd, Rm, imm);
}

void _add_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rm = ins_code >> 6 & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);

	_add_reg(Rm, Rn, Rd, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_add_reg_16, R%d,R%d,R%d\n", Rd, Rn, Rm);	
}

void _sub_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rm = ins_code >> 6 & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);

	_sub_reg(Rm, Rn, Rd, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_sub_reg_16, R%d,R%d,R%d\n", Rd, Rn, Rm);	
}

void _add_imm3_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t imm32 = ins_code >> 6 & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);

	_add_imm(imm32, Rn, Rd, setflags, regs);	
	LOG_INSTRUCTION("_add_imm3_16, R%d,R%d,#%d\n", Rd, Rn, imm32);	
}

void _sub_imm3_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t imm32 = ins_code >> 6 & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);

	_sub_imm(imm32, Rn, Rd, setflags, regs);	
	LOG_INSTRUCTION("_add_imm3_16, R%d,R%d,#%d\n", Rd, Rn, imm32);		
}

void _mov_imm_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rd = ins_code >> 8 & 0x7;
	uint32_t imm = ins_code & 0xFF;
	uint32_t setflags = !InITBlock(regs);
	uint32_t carry = GET_APSR_C(regs);

	_mov_imm(Rd, imm, setflags, carry, regs);
	LOG_INSTRUCTION("_mov_imm_16, R%d,#%d\n", Rd, imm);
}

void _cmp_imm_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rn = ins_code >> 8 & 0x7;
	uint32_t imm8 = ins_code & 0xFF;

	_cmp_imm(imm8, Rn, regs);
	LOG_INSTRUCTION("_cmp_imm_16, R%d,#%d\n", Rn, imm8);
}

void _add_imm8_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rn = ins_code >> 8 & 0x7;
	uint32_t Rd = Rn;
	uint32_t imm8 = ins_code & 0xFF;
	int setflags = !InITBlock(regs);

	_add_imm(imm8, Rn, Rd, setflags, regs);
	LOG_INSTRUCTION("_add_imm8_16, R%d,#%d\n", Rn, imm8);
}

void _sub_imm8_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_and_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_and_reg_16, R%d,R%d\n", Rdn, Rm);
}

void _eor_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_eor_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_eor_reg_16, R%d,R%d\n", Rdn, Rm);
}

void _lsl_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_lsl_reg(Rm, Rdn, Rdn, setflags, regs);
	LOG_INSTRUCTION("_lsl_reg_16, R%d,R%d\n", Rdn, Rm);
}

void _lsr_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_lsr_reg(Rm, Rdn, Rdn, setflags, regs);
	LOG_INSTRUCTION("_lsr_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _asr_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_asr_reg(Rm, Rdn, Rdn, setflags, regs);
	LOG_INSTRUCTION("_asr_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _adc_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_adc_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_adc_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _sbc_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_sbc_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_sbc_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _ror_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_ror_reg(Rm, Rdn, Rdn, setflags, regs);
	LOG_INSTRUCTION("_ror_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _tst_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;

	_tst_reg(Rm, Rn, SRType_LSL, 0, regs);
	LOG_INSTRUCTION("_tst_reg_16 R%d,R%d\n", Rn, Rm);
}

void _rsb_imm_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rd = ins_code & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);
	uint32_t imm32 = 0;

	_rsb_imm(imm32, Rn, Rd, setflags, regs);
	LOG_INSTRUCTION("_rsb_imm_16 R%d,R%d,#0\n", Rd, Rn);
}

void _cmp_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;

	_cmp_reg(Rm, Rn, SRType_LSL, 0, regs);
	LOG_INSTRUCTION("_cmp_reg_16 R%d,R%d\n", Rn, Rm);
}

void _cmn_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;

	_cmn_reg(Rm, Rn, SRType_LSL, 0, regs); 
	LOG_INSTRUCTION("_cmn_reg_16 R%d,R%d\n", Rn, Rm);
}

void _orr_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_orr_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_orr_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _mul_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rdm = ins_code & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_mul_reg(Rdm, Rn, Rdm, setflags, regs);
	LOG_INSTRUCTION("_mul_reg_16 R%d,R%d,R%d\n", Rdm, Rn, Rdm);
}

void _bic_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_bic_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_bic_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _mvn_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rd = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_mvn_reg(Rm, Rd, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_mvn_reg_16 R%d,R%d\n", Rd, Rm);
}

void _add_sp_reg_T1(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rm = ins_code >> 3 & 0xFul;
	uint32_t Rd = SP_INDEX;
	assert(Rm != 13);

	_add_sp_reg(Rm, Rd, SRType_LSL, 0, 0, regs);
	LOG_INSTRUCTION("_add_sp_reg_T1 SP,R%d\n", Rm);
}

void _add_reg_spec_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rt, Rn, Rm;
	decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);
	_str_reg(Rm, Rn, Rt, SRType_LSL, 0, cpu);
	LOG_INSTRUCTION("_str_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _strh_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rt, Rn, Rm;
	decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);

	_strh_reg(Rm, Rn, Rt, SRType_LSL, 0, cpu);
	LOG_INSTRUCTION("_strh_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _strb_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rt, Rn, Rm;
	decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);

	_strb_reg(Rm, Rn, Rt, SRType_LSL, 0, cpu);
	LOG_INSTRUCTION("_strb_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _ldrsb_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rt, Rn, Rm;
	decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);

	bool_t add = TRUE;
	bool_t index = TRUE;
	_ldrsb_reg(Rm, Rn, Rt, add, index, SRType_LSL, 0, cpu);
	LOG_INSTRUCTION("_ldrsb_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _ldr_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rt, Rn, Rm;
	decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);
	bool_t index = TRUE;
	bool_t add = TRUE;

	_ldrb_reg(Rm, Rn, Rt, add, index, SRType_LSL, 0, cpu);
	LOG_INSTRUCTION("_ldrb_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _ldrsh_reg_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t imm8 = LOW_BIT16(ins_code, 8) << 2;
	uint32_t Rd = LOW_BIT16(ins_code>>8, 3);
	bool_t add = TRUE;

	_adr(imm8, Rd, add, regs);
	LOG_INSTRUCTION("_adr_16 R%d,#%d\n", Rd, imm8);
}

void _add_sp_imm_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t imm8 = LOW_BIT16(ins_code, 8) << 2;
	uint32_t Rd = LOW_BIT16(ins_code>>8, 3);
	uint32_t setflags = FALSE;

	_add_sp_imm(imm8, Rd, setflags, regs);
	LOG_INSTRUCTION("_add_sp_imm_16 R%d,sp,#%d\n", Rd, imm8);
}

void _add_sp_sp_imm_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t imm7 = LOW_BIT16(ins_code, 7) << 2;
	uint32_t Rd = 13;
	uint32_t setflags = FALSE;

	_add_sp_imm(imm7, Rd, setflags, regs);
	LOG_INSTRUCTION("_add_sp_sp_imm_16 sp,sp,#%d\n", imm7);
}

void _sub_sp_sp_imm_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t imm7 = LOW_BIT16(ins_code, 7) << 2;
	uint32_t Rd = 13;
	uint32_t setflags = FALSE;

	_sub_sp_imm(imm7, Rd, setflags, regs);
	LOG_INSTRUCTION("_sub_sp_sp_imm_16 sp,sp,#%d\n", imm7);
}

void _cbnz_cbz_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rd, Rm;
	decode_rm_rd_16(ins_code, &Rm, &Rd);
	uint32_t rotation = 0;

	_sxth(Rm, Rd, rotation, regs);
	LOG_INSTRUCTION("_sxth_16 R%d,R%d\n", Rd, Rm);
}

void _sxtb_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rd, Rm;
	decode_rm_rd_16(ins_code, &Rm, &Rd);
	uint32_t rotation = 0;

	_sxtb(Rm, Rd, rotation, regs);
	LOG_INSTRUCTION("_sxtb_16 R%d,R%d\n", Rd, Rm);
}

void _uxth_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rd, Rm;
	decode_rm_rd_16(ins_code, &Rm, &Rd);
	uint32_t rotation = 0;

	_uxth(Rm, Rd, rotation, regs);
	LOG_INSTRUCTION("_uxth_16 R%d,R%d\n", Rd, Rm);
}

void _uxtb_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rd, Rm;
	decode_rm_rd_16(ins_code, &Rm, &Rd);
	uint32_t rotation = 0;

	_uxtb(Rm, Rd, rotation, regs);
	LOG_INSTRUCTION("_uxtb_16 R%d,R%d\n", Rd, Rm);
}

void _push_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t register_list = LOW_BIT16(ins_code, 7);
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rm, Rd;
	decode_rm_rd_16(ins_code, &Rm, &Rd);

	_rev(Rm, Rd, regs);
	LOG_INSTRUCTION("_rev_16, R%d,R%d\n", Rd, Rm);
}

void _rev16_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rm, Rd;
	decode_rm_rd_16(ins_code, &Rm, &Rd);

	_rev16(Rm, Rd, regs);
	LOG_INSTRUCTION("_rev16_16, R%d,R%d\n", Rd, Rm);
}

void _revsh_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t Rm, Rd;
	decode_rm_rd_16(ins_code, &Rm, &Rd);
	
	_revsh(Rm, Rd, regs);
	LOG_INSTRUCTION("revsh_16, R%d,R%d\n", Rd, Rm);
}

void _pop_16(uint16_t ins_code, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t register_list = LOW_BIT16(ins_code, 7);
	uint32_t registers = LOW_BIT16(ins_code >> 8, 1) << 14 | register_list; 
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
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	uint32_t imm32 = LOW_BIT16(ins_code, 8);

	//TODO: bkpt event 
	LOG_INSTRUCTION("_bkpt_16 #%d\n", imm32);
}

void _it_16(uint32_t opA, uint32_t opB, cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	armv7m_state* state = (armv7m_state*)cpu->run_info.cpu_spec_info;
	uint32_t mask = opB;
	uint32_t firstcond = opA;
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

void _it_hint_16(uint16_t ins_code, cpu_t* cpu)
{
	uint32_t opB = LOW_BIT16(ins_code, 4);
	uint32_t opA = LOW_BIT16(ins_code>>4, 4);

	if(opB != 0){
		_it_16(opA, opB, cpu);
	}else{
		switch(opA){
		case 0x0:
			/* NOP: do nothing */
			LOG_INSTRUCTION("NOP\n");
			break;
		case 0x1:
			/* YIELD: TODO: */
			LOG_INSTRUCTION("YIELD treated as NOP\n");
			break;
		case 0x2:
			/* WFE: wait for event */
			LOG_INSTRUCTION("WFE treated as NOP\n");
			break;
		case 0x3:
			/* WFI: wait for interrupt */
			LOG_INSTRUCTION("WFI treated as NOP\n");
			break;
		case 0x4:
			/* SEV: send event hint */
			LOG_INSTRUCTION("SEV treated as NOP\n");
			break;
		default:
			break;
		}
	}
}

void _unpredictable_16(uint16_t ins_code, cpu_t* cpu)
{
	LOG(LOG_WARN, "ins_code: %x is UNPREDICTABLE, treated as NOP\n", ins_code);
}

/****** init instruct table ******/
void init_instruction_table(armv7m_instruct_table_t* table)
{
	// 15~10 bit A5-156
	set_base_table_value(table->base_table16, 0x00, 0x0F, (armv7m_translate16_t)shift_add_sub_mov);		// 00xxxx		
	set_base_table_value(table->base_table16, 0x10, 0x10, (armv7m_translate16_t)data_process);			// 010000				
	set_base_table_value(table->base_table16, 0x11, 0x11, (armv7m_translate16_t)spdata_branch_exchange);	// 010001
	set_base_table_value(table->base_table16, 0x12, 0x13, (armv7m_translate16_t)load_literal_pool);		// 01001x
	set_base_table_value(table->base_table16, 0x14, 0x27, (armv7m_translate16_t)load_store_single);		// 0101xx 011xxx 100xxx
	set_base_table_value(table->base_table16, 0x28, 0x29, (armv7m_translate16_t)pc_related_address);		// 10100x
	set_base_table_value(table->base_table16, 0x2A, 0x2B, (armv7m_translate16_t)sp_related_address);		// 10101x
	set_base_table_value(table->base_table16, 0x2C, 0x2F, (armv7m_translate16_t)misc_16bit_ins);			// 1011xx
	//base_table[0x30~0x31] = store_multiple			// 11000x
	//base_table[0x32~0x33] = load_multiple			// 11001x
	//base_table[0x34~0x37] = con_branch_super_call	// 1101xx
	//base_table[0x38~0x39] = uncon_branch			// 11100x

	// shift(imm) add sub mov A5-157
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x00, 0x03, (armv7m_translate16_t)_lsl_imm_16);	// 000xx
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x04, 0x07, (armv7m_translate16_t)_lsr_imm_16);	// 001xx
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x08, 0x0B, (armv7m_translate16_t)_asr_imm_16);	// 010xx
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x0C, 0x0C, (armv7m_translate16_t)_add_reg_16);	// 01100
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x0D, 0x0D, (armv7m_translate16_t)_sub_reg_16);	// 01101
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x0E, 0x0E, (armv7m_translate16_t)_add_imm3_16);	// 01110
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x0F, 0x0F, (armv7m_translate16_t)_sub_imm3_16);	// 01111
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x10, 0x13, (armv7m_translate16_t)_mov_imm_16);	// 100xx
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x14, 0x17, (armv7m_translate16_t)_cmp_imm_16);	// 101xx
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x18, 0x1B, (armv7m_translate16_t)_add_imm8_16);	// 110xx
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x1C, 0x1F, (armv7m_translate16_t)_sub_imm8_16);	// 111xx

	// data processing A5-158
	set_sub_table_value(table->data_process_table16, 0x00, 0x00, (armv7m_translate16_t)_and_reg_16);		// 0000
	set_sub_table_value(table->data_process_table16, 0x01, 0x01, (armv7m_translate16_t)_eor_reg_16);		// 0001
	set_sub_table_value(table->data_process_table16, 0x02, 0x02, (armv7m_translate16_t)_lsl_reg_16);		// 0010
	set_sub_table_value(table->data_process_table16, 0x03, 0x03, (armv7m_translate16_t)_lsr_reg_16);		// 0011
	set_sub_table_value(table->data_process_table16, 0x04, 0x04, (armv7m_translate16_t)_asr_reg_16);		// 0100
	set_sub_table_value(table->data_process_table16, 0x05, 0x05, (armv7m_translate16_t)_adc_reg_16);		// 0101
	set_sub_table_value(table->data_process_table16, 0x06, 0x06, (armv7m_translate16_t)_sbc_reg_16);		// 0110
	set_sub_table_value(table->data_process_table16, 0x07, 0x07, (armv7m_translate16_t)_ror_reg_16);		// 0111
	set_sub_table_value(table->data_process_table16, 0x08, 0x08, (armv7m_translate16_t)_tst_reg_16);		// 1000
	set_sub_table_value(table->data_process_table16, 0x09, 0x09, (armv7m_translate16_t)_rsb_imm_16);		// 1001
	set_sub_table_value(table->data_process_table16, 0x0A, 0x0A, (armv7m_translate16_t)_cmp_reg_16);		// 1010
	set_sub_table_value(table->data_process_table16, 0x0B, 0x0B, (armv7m_translate16_t)_cmn_reg_16);		// 1011
	set_sub_table_value(table->data_process_table16, 0x0C, 0x0C, (armv7m_translate16_t)_orr_reg_16);		// 1100
	set_sub_table_value(table->data_process_table16, 0x0D, 0x0D, (armv7m_translate16_t)_mul_reg_16);		// 1101
	set_sub_table_value(table->data_process_table16, 0x0E, 0x0E, (armv7m_translate16_t)_bic_reg_16);		// 1110
	set_sub_table_value(table->data_process_table16, 0x0F, 0x0F, (armv7m_translate16_t)_mvn_reg_16);		// 1111

	// special data instructions and branch and exchange A5-159
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x00, 0x03, (armv7m_translate16_t)_add_reg_spec_16);	// 00xx
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x04, 0x04, (armv7m_translate16_t)_unpredictable_16);	// 0100(*)
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x05, 0x07, (armv7m_translate16_t)_cmp_reg_spec_16);	// 0101,011x
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x08, 0x0B, (armv7m_translate16_t)_mov_reg_spec_16);	// 10xx
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x0C, 0x0D, (armv7m_translate16_t)_bx_spec_16);		// 110x
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x0E, 0x0F, (armv7m_translate16_t)_blx_spec_16);		// 111x

	// load from literal pool A7-289
	set_sub_table_value(table->load_literal_table16, 0x00, 0x00, (armv7m_translate16_t)_ldr_literal_16);

	// load store single data item A5-160
	set_sub_table_value(table->load_store_single_table16, 0x00, 0x27, (armv7m_translate16_t)_unpredictable_16);		// 0000 000 ~ 0010 011
	set_sub_table_value(table->load_store_single_table16, 0x28, 0x28, (armv7m_translate16_t)_str_reg_16);				// 0101 000
	set_sub_table_value(table->load_store_single_table16, 0x29, 0x29, (armv7m_translate16_t)_strh_reg_16);			// 0101 001
	set_sub_table_value(table->load_store_single_table16, 0x2A, 0x2A, (armv7m_translate16_t)_strb_reg_16);			// 0101 010
	set_sub_table_value(table->load_store_single_table16, 0x2B, 0x2B, (armv7m_translate16_t)_ldrsb_reg_16);			// 0101 011
	set_sub_table_value(table->load_store_single_table16, 0x2C, 0x2C, (armv7m_translate16_t)_ldr_reg_16);				// 0101	100
	set_sub_table_value(table->load_store_single_table16, 0x2D, 0x2D, (armv7m_translate16_t)_ldrh_reg_16);			// 0101 101
	set_sub_table_value(table->load_store_single_table16, 0x2E, 0x2E, (armv7m_translate16_t)_ldrb_reg_16);			// 0101 110
	set_sub_table_value(table->load_store_single_table16, 0x2F, 0x2F, (armv7m_translate16_t)_ldrsh_reg_16);			// 0101 111
	set_sub_table_value(table->load_store_single_table16, 0x2F, 0x2F, (armv7m_translate16_t)_ldrsh_reg_16);			// 0101 111
	set_sub_table_value(table->load_store_single_table16, 0x30, 0x33, (armv7m_translate16_t)_str_imm_16);				// 0110 0xx
	set_sub_table_value(table->load_store_single_table16, 0x34, 0x37, (armv7m_translate16_t)_ldr_imm_16);				// 0110 1xx
	set_sub_table_value(table->load_store_single_table16, 0x38, 0x3B, (armv7m_translate16_t)_strb_imm_16);			// 0111 0xx
	set_sub_table_value(table->load_store_single_table16, 0x3C, 0x3F, (armv7m_translate16_t)_ldrb_imm_16);			// 0111 1xx
	set_sub_table_value(table->load_store_single_table16, 0x40, 0x43, (armv7m_translate16_t)_strh_imm_16);			// 1000 0xx
	set_sub_table_value(table->load_store_single_table16, 0x44, 0x47, (armv7m_translate16_t)_ldrh_imm_16);			// 1000 1xx
	set_sub_table_value(table->load_store_single_table16, 0x48, 0x4B, (armv7m_translate16_t)_str_sp_imm_16);			// 1001 0xx
	set_sub_table_value(table->load_store_single_table16, 0x4C, 0x4F, (armv7m_translate16_t)_ldr_sp_imm_16);			// 1001 1xx

	// pc related address A7-229
	set_sub_table_value(table->pc_related_address_table16, 0x00, 0x00, (armv7m_translate16_t)_adr_16);

	// sp related address A7-225
	set_sub_table_value(table->sp_related_address_table16, 0x00, 0x00, (armv7m_translate16_t)_add_sp_imm_16);

	// misc 16-bit instructions A5-161
	set_sub_table_value(table->misc_16bit_ins_table, 0x00, MISC_16BIT_INS_SIZE-1, (armv7m_translate16_t)_unpredictable_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x00, 0x03, (armv7m_translate16_t)_add_sp_sp_imm_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x04, 0x07, (armv7m_translate16_t)_sub_sp_sp_imm_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x08, 0x0F, (armv7m_translate16_t)_cbnz_cbz_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x10, 0x11, (armv7m_translate16_t)_sxth_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x12, 0x13, (armv7m_translate16_t)_sxtb_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x14, 0x15, (armv7m_translate16_t)_uxth_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x16, 0x17, (armv7m_translate16_t)_uxtb_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x18, 0x1F, (armv7m_translate16_t)_cbnz_cbz_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x20, 0x2F, (armv7m_translate16_t)_push_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x48, 0x4F, (armv7m_translate16_t)_cbnz_cbz_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x50, 0x51, (armv7m_translate16_t)_rev_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x52, 0x53, (armv7m_translate16_t)_rev16_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x56, 0x57, (armv7m_translate16_t)_revsh_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x58, 0x5F, (armv7m_translate16_t)_cbnz_cbz_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x60, 0x67, (armv7m_translate16_t)_pop_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x70, 0x77, (armv7m_translate16_t)_bkpt_16);
	set_sub_table_value(table->misc_16bit_ins_table, 0x78, 0x7F, (armv7m_translate16_t)_it_hint_16);
}



bool_t is_16bit_code(uint16_t opcode)
{
	uint16_t opcode_header = opcode >> 13;
	if(opcode_header == 0x7){
		return FALSE;
	}else{
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
armv7m_translate16_t armv7m_parse_opcode16(uint16_t opcode, cpu_t* cpu)
{
	return (armv7m_translate16_t)translate_table.base_table16[opcode >> 10](opcode, cpu);
}

//void _armv7m_parse_opcode16(uint16_t opcode, run_info_t* run_info, memory_map_t* memory)
//{
//	armv7m_instruct_t* ins = (armv7m_instruct_t*)run_info->ins_set;
//	ins->ins_table->base_table16[opcode >> 10](opcode, run_info, memory);
//
//	switch(opcode>>11){
//	case 0x00:
//		// lsl imm
//	case 0x01:
//		// lsr imm
//	case 0x02:
//		// asr imm
//	case 0x03:
//		switch((opcode>>9) & 0x3ul){
//		case 0x0:
//			// add reg
//		case 0x1:
//			// sub reg
//		case 0x2:
//			// add imm3
//		case 0x3:
//			// sub imm3
//		}
//		break;
//	case 0x04:
//		// mov imm
//	case 0x05:
//		// cmp imm
//	case 0x06:
//		// add imm8
//	case 0x07:
//		// sub imm8
//	case 0x08:
//		switch((opcode>>6) & 0x1Ful){
//		case 0x00:
//			//and reg
//		case 0x01:
//			//eor reg
//		case 0x02:
//			// lsl reg
//		}
//		break;
//	}
//}

void parse_opcode32(uint32_t opcode, armv7m_instruct_t* ins)
{
//	ins->ins_table->base_table32[opcode >> 13](opcode, ins);
}

void armv7m_next_PC_16(armv7m_reg_t* regs)
{
	regs->PC += 2;
	regs->PC_return = regs->PC + 2;
}

void armv7m_next_PC_32(armv7m_reg_t* regs)
{
	regs->PC += 4;
	regs->PC_return = regs->PC;
}

void armv7m_next_PC(cpu_t* cpu, int ins_length)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs; 
	if(ins_length == 16){
		armv7m_next_PC_16(regs);
	}else if(ins_length == 32){
		armv7m_next_PC_32(regs);
	}
	cpu->run_info.last_pc = regs->PC;
}

int armv7m_PC_modified(cpu_t* cpu)
{
	armv7m_reg_t* regs = (armv7m_reg_t*)cpu->regs;
	return (uint32_t)cpu->run_info.last_pc != regs->PC;
}

armv7m_reg_t* create_armv7m_regs()
{
	armv7m_reg_t* regs = (armv7m_reg_t*)calloc(1, sizeof(armv7m_reg_t));
	
	return regs;
}

error_code_t destory_armv7m_regs(armv7m_reg_t** regs)
{
	if(regs == NULL || *regs == NULL){
		return ERROR_NULL_POINTER;
	}

	free(*regs);
	*regs = NULL;

	return SUCCESS;
}

armv7m_state* create_armv7m_state()
{
	return (armv7m_state*)malloc(sizeof(armv7m_state))	;
}

error_code_t destory_armv7m_state(armv7m_state** state)
{
	if(state == NULL || *state == NULL){
		return ERROR_NULL_POINTER;
	}

	free(*state);
	*state = NULL;

	return SUCCESS;
}

/* create and initialize the instruction as well as the cpu state */
error_code_t ins_armv7m_init(_IO cpu_t* cpu)
{
	armv7m_state* state = create_armv7m_state();
	if(state == NULL){
		goto state_error;
	}
	set_cpu_spec_info(cpu, state);
	cpu->regs = create_armv7m_regs();
	if(cpu->regs == NULL){
		goto regs_error;
	}
	init_instruction_table(&translate_table);
	cpu->instruction_data = &translate_table;
	return SUCCESS;

regs_error:
	destory_armv7m_state((armv7m_state**)&cpu->run_info.cpu_spec_info);
state_error:
	return ERROR_CREATE;
}

error_code_t ins_armv7m_destory(cpu_t* cpu)
{
	destory_armv7m_regs((armv7m_reg_t**)&cpu->regs);
	destory_armv7m_state((armv7m_state**)&cpu->run_info.cpu_spec_info);
	cpu->instruction_data = NULL;

	return SUCCESS;
}