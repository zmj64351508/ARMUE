#include "_types.h"
#include "arm_v7m_ins_decode.h"
#include "cpu.h"
#include <stdlib.h>
#include <assert.h>

/******							IMPROTANT									*/
/****** PC always pointers to the address of next instruction.				*/
/****** when 16bit coded, PC += 2. when 32bit coded, PC += 4.				*/		
/****** But, when 16 bit coded instruction visit PC, it will return PC+2	*/


error_code_t set_base_table_value(sub_translate16_t* table, int start, int end, sub_translate16_t value)
{
	for(int i = start; i <= end; i++){
		table[i] = value;
	}

	return SUCCESS;
}

error_code_t set_sub_table_value(instruction16_t* table, int start, int end, instruction16_t value)
{
	for(int i = start; i <= end; i++){
		table[i] = value;
	}

	return SUCCESS;
}

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
void shift_add_sub_mov(uint16_t opcode, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_instruct_t* ins = (armv7m_instruct_t*)run_info->ins_set;
	// call appropriate instruction implement function by inquiring 13~9bit
	ins->ins_table->shift_add_sub_mov_table16[opcode >> 9 & 0x1F](opcode, run_info, memory);
}

void data_process(uint16_t opcode, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_instruct_t* ins = (armv7m_instruct_t*)run_info->ins_set;
	// call appropriate instruction implement function by inquiring 9~6bit
	ins->ins_table->data_process_table16[opcode >> 6 & 0x0F](opcode, run_info, memory);	
}

void spdata_branch_exchange(uint16_t opcode, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_instruct_t* ins = (armv7m_instruct_t*)run_info->ins_set;
	// call appropriate instruction implement function by inquiring 9~6bit
	ins->ins_table->spdata_branch_exchange_table16[opcode >> 6 & 0x0F](opcode, run_info, memory);
}

void load_literal_pool(uint16_t opcode, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_instruct_t* ins = (armv7m_instruct_t*)run_info->ins_set;
	/* load fron literal pool set has only 1 instruction */
	ins->ins_table->load_literal_table16[0](opcode, run_info, memory);
}

void load_store_single(uint16_t opcode, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_instruct_t* ins = (armv7m_instruct_t*)run_info->ins_set;
	// call appropriate instruction implement function by inquiring 15~9bit
	ins->ins_table->load_store_single_table16[opcode >> 9 & 0x7F](opcode, run_info, memory);
}

void pc_related_address(uint16_t opcode, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_instruct_t* ins = (armv7m_instruct_t*)run_info->ins_set;
	ins->ins_table->pc_related_address_table16[0](opcode, run_info, memory);
}

void sp_related_address(uint16_t opcode, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_instruct_t* ins = (armv7m_instruct_t*)run_info->ins_set;
	ins->ins_table->sp_related_address_table16[0](opcode, run_info, memory);
}

void misc_16bit_ins(uint16_t opcode, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_instruct_t* ins = (armv7m_instruct_t*)run_info->ins_set;
	ins->ins_table->misc_16bit_ins_table[opcode >> 5 & 0x7F](opcode, run_info, memory);
}

#define GET_ITSTATE(regs) ((regs->xPSR >> 8 & 0xFC) | (regs->xPSR >> 25 & 0x3))

uint32_t InITBlock(armv7m_reg_t* regs)
{
	uint8_t ITstate = GET_ITSTATE(regs);
	return (ITstate & 0xF) != 0;
}

uint32_t LastInITBlock(armv7m_reg_t* regs)
{
	uint8_t ITstate = GET_ITSTATE(regs);
	return (ITstate & 0xF) == 0x8;
}

/****** Here are the final decoders of 16bit instructions ******/
void _lsl_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t imm = ins_code >> 6 & 0x1F;
	uint32_t Rm = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);

	_lsl_imm(imm, Rm, Rd, setflags, regs);
	LOG_INSTRUCTION("_lsl_imm_16, R%d,R%d,#%d\n", Rd, Rm, imm);
}

void _lsr_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t imm = ins_code >> 6 & 0x1F;
	uint32_t Rm = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);
	
	_lsr_imm(imm, Rm, Rd, setflags, regs);		
	LOG_INSTRUCTION("_lsr_imm_16, R%d,R%d,#%d\n", Rd, Rm, imm);
}

void _asr_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t imm = ins_code >> 6 & 0x1F;
	uint32_t Rm = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);

	_asr_imm(imm, Rm, Rd, setflags, regs);	
	LOG_INSTRUCTION("_asr_imm_16, R%d,R%d,#%d\n", Rd, Rm, imm);
}

void _add_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rm = ins_code >> 6 & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);

	_add_reg(Rm, Rn, Rd, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_add_reg_16, R%d,R%d,R%d\n", Rd, Rn, Rm);	
}

void _sub_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rm = ins_code >> 6 & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);

	_sub_reg(Rm, Rn, Rd, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_sub_reg_16, R%d,R%d,R%d\n", Rd, Rn, Rm);	
}

void _add_imm3_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t imm32 = ins_code >> 6 & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);

	_add_imm(imm32, Rn, Rd, setflags, regs);	
	LOG_INSTRUCTION("_add_imm3_16, R%d,R%d,#%d\n", Rd, Rn, imm32);	
}

void _sub_imm3_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t imm32 = ins_code >> 6 & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflags = !InITBlock(regs);

	_sub_imm(imm32, Rn, Rd, setflags, regs);	
	LOG_INSTRUCTION("_add_imm3_16, R%d,R%d,#%d\n", Rd, Rn, imm32);		
}

void _mov_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rd = ins_code >> 8 & 0x7;
	uint32_t imm = ins_code & 0xFF;
	uint32_t setflags = !InITBlock(regs);
	uint32_t carry = GET_APSR_C(regs);

	_mov_imm(Rd, imm, setflags, carry, regs);
	LOG_INSTRUCTION("_mov_imm_16, R%d,#%d\n", Rd, imm);
}

void _cmp_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rn = ins_code >> 8 & 0x7;
	uint32_t imm8 = ins_code & 0xFF;

	_cmp_imm(imm8, Rn, regs);
	LOG_INSTRUCTION("_cmp_imm_16, R%d,#%d\n", Rn, imm8);
}

void _add_imm8_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rn = ins_code >> 8 & 0x7;
	uint32_t Rd = Rn;
	uint32_t imm8 = ins_code & 0xFF;
	int setflags = !InITBlock(regs);

	_add_imm(imm8, Rn, Rd, setflags, regs);
	LOG_INSTRUCTION("_add_imm8_16, R%d,#%d\n", Rn, imm8);
}

void _sub_imm8_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rn = ins_code >> 8 & 0x7;
	uint32_t Rd = Rn;
	uint32_t imm8 = ins_code & 0xFF;
	int setflags = !InITBlock(regs);	
	
	_sub_imm(imm8, Rn, Rd, setflags, regs);
	LOG_INSTRUCTION("_sub_imm8_16, R%d,#%d\n", Rn, imm8);
}

/************************/
void _and_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_and_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_and_reg_16, R%d,R%d\n", Rdn, Rm);
}

void _eor_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_eor_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_eor_reg_16, R%d,R%d\n", Rdn, Rm);
}

void _lsl_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_lsl_reg(Rm, Rdn, Rdn, setflags, regs);
	LOG_INSTRUCTION("_lsl_reg_16, R%d,R%d\n", Rdn, Rm);
}

void _lsr_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_lsr_reg(Rm, Rdn, Rdn, setflags, regs);
	LOG_INSTRUCTION("_lsr_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _asr_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_asr_reg(Rm, Rdn, Rdn, setflags, regs);
	LOG_INSTRUCTION("_asr_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _adc_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_adc_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_adc_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _sbc_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_sbc_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_sbc_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _ror_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_ror_reg(Rm, Rdn, Rdn, setflags, regs);
	LOG_INSTRUCTION("_ror_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _tst_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;

	_tst_reg(Rm, Rn, SRType_LSL, 0, regs);
	LOG_INSTRUCTION("_tst_reg_16 R%d,R%d\n", Rn, Rm);
}

void _rsb_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rd = ins_code & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);
	uint32_t imm32 = 0;

	_rsb_imm(imm32, Rn, Rd, setflags, regs);
	LOG_INSTRUCTION("_rsb_imm_16 R%d,R%d,#0\n", Rd, Rn);
}

void _cmp_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;

	_cmp_reg(Rm, Rn, SRType_LSL, 0, regs);
	LOG_INSTRUCTION("_cmp_reg_16 R%d,R%d\n", Rn, Rm);
}

void _cmn_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;

	_cmn_reg(Rm, Rn, SRType_LSL, 0, regs); 
	LOG_INSTRUCTION("_cmn_reg_16 R%d,R%d\n", Rn, Rm);
}

void _orr_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_orr_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_orr_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _mul_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rdm = ins_code & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_mul_reg(Rdm, Rn, Rdm, setflags, regs);
	LOG_INSTRUCTION("_mul_reg_16 R%d,R%d,R%d\n", Rdm, Rn, Rdm);
}

void _bic_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_bic_reg(Rm, Rdn, Rdn, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_bic_reg_16 R%d,R%d\n", Rdn, Rm);
}

void _mvn_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rd = ins_code & 0x7;
	uint32_t Rm = ins_code >> 3 & 0x7;
	int setflags = !InITBlock(regs);

	_mvn_reg(Rm, Rd, SRType_LSL, 0, setflags, regs);
	LOG_INSTRUCTION("_mvn_reg_16 R%d,R%d\n", Rd, Rm);
}

void _add_sp_reg_T1(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t DM = ins_code >> 7 & 0x1ul;
	uint32_t Rdm = (DM << 3) | (ins_code & 0x7ul);
	
	if(Rdm == 15 && InITBlock(regs) && !LastInITBlock(regs)){
		LOG_INSTRUCTION("_add_sp_reg_T1 R%d,SP,R%d as UNPREDICTABLE\n", Rdm, Rdm);
	}else{
		_add_sp_reg(Rdm, Rdm, SRType_LSL, 0, 0, regs);
		LOG_INSTRUCTION("_add_sp_reg_T1 R%d,SP,R%d\n", Rdm, Rdm);
	}
}

void _add_sp_reg_T2(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t Rm = ins_code >> 3 & 0xFul;
	uint32_t Rd = SP_INDEX;
	assert(Rm != 13);

	_add_sp_reg(Rm, Rd, SRType_LSL, 0, 0, regs);
	LOG_INSTRUCTION("_add_sp_reg_T1 SP,R%d\n", Rm);
}

void _add_reg_spec_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t DN = ins_code >> 7 & 0x1ul;
	uint32_t Rdn = (DN << 3) | (ins_code & 0x7ul);
	uint32_t Rm = ins_code >> 3 & 0xFul;

	if(Rm == 13){
		_add_sp_reg_T1(ins_code, run_info, memory);
	}else if(Rdn == 13){
		_add_sp_reg_T2(ins_code, run_info, memory);
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

void _cmp_reg_spec_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
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

void _mov_reg_spec_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
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

void _bx_spec_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t Rm = ins_code >> 3 & 0xFul;
	if((ins_code & 0x7ul) != 0 || 
	   (InITBlock(regs) && !LastInITBlock(regs))){
		LOG_INSTRUCTION("_bx_spec_16 R%d as UNPREDICTABLE\n", Rm);
	}else{
		_bx(Rm, regs, state);
		LOG_INSTRUCTION("_bx_spec_16 R%d\n", Rm);
	}
}

void _blx_spec_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t Rm = ins_code >> 3 & 0xFul;
	if((ins_code & 0x7ul) != 0 ||
	   (InITBlock(regs) && !LastInITBlock(regs))){
		LOG_INSTRUCTION("_blx_spec_16 R%d as UNPREDICTABLE\n", Rm);
	}else{
		_blx(Rm, regs,state);
		LOG_INSTRUCTION("_blx_spec_16 R%d\n", Rm);
	}
}

void _ldr_literal_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t imm8 = (ins_code & 0xFFul) << 2;
	uint32_t Rt = ins_code >> 8 & 0x7ul;
	bool_t add = TRUE;

	_ldr_literal(imm8, Rt, add, regs, state, memory);
	LOG_INSTRUCTION("_ldr_literal_16 R%d,[PC,#%d]\n", Rt, imm8);
}

inline void decode_ldr_str_reg_16(uint16_t ins_code, _O uint32_t* Rm, _O uint32_t* Rt, _O uint32_t* Rn)
{
	*Rt = LOW_BIT16(ins_code, 3);
	*Rn = LOW_BIT16(ins_code>>3, 3);
	*Rm = LOW_BIT16(ins_code>>6, 3);
}

void _str_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t Rt, Rn, Rm;
	decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);
	_str_reg(Rm, Rn, Rt, SRType_LSL, 0, regs, state, memory);
	LOG_INSTRUCTION("_str_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _strh_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t Rt, Rn, Rm;
	decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);

	_strh_reg(Rm, Rn, Rt, SRType_LSL, 0, regs, state, memory);
	LOG_INSTRUCTION("_strh_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _strb_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t Rt, Rn, Rm;
	decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);

	_strb_reg(Rm, Rn, Rt, SRType_LSL, 0, regs, state, memory);
	LOG_INSTRUCTION("_strb_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _ldrsb_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t Rt, Rn, Rm;
	decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);

	bool_t add = TRUE;
	bool_t index = TRUE;
	_ldrsb_reg(Rm, Rn, Rt, add, index, SRType_LSL, 0, regs, state, memory);
	LOG_INSTRUCTION("_ldrsb_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _ldr_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t Rt, Rn, Rm;
	decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);
	bool_t index = TRUE;
	bool_t add = TRUE;
	bool_t wback = FALSE;

	_ldr_reg(Rm, Rn, Rt, add, index, wback, SRType_LSL, 0, regs, state, memory);
	LOG_INSTRUCTION("_ldr_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _ldrh_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t Rt, Rn, Rm;
	decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);
	bool_t index = TRUE;
	bool_t add = TRUE;
	bool_t wback = FALSE;

	_ldrh_reg(Rm, Rn, Rt, add, index, wback, SRType_LSL, 0, regs, state, memory);
	LOG_INSTRUCTION("_ldrh_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _ldrb_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t Rt, Rn, Rm;
	decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);
	bool_t index = TRUE;
	bool_t add = TRUE;

	_ldrb_reg(Rm, Rn, Rt, add, index, SRType_LSL, 0, regs, state, memory);
	LOG_INSTRUCTION("_ldrb_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

void _ldrsh_reg_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t Rt, Rn, Rm;
	decode_ldr_str_reg_16(ins_code, &Rm, &Rt, &Rn);
	bool_t index = TRUE;
	bool_t add = TRUE;
	bool_t wback = FALSE;

	_ldrsh_reg(Rm, Rn, Rt, add, index, wback, SRType_LSL, 0, regs, state, memory);
	LOG_INSTRUCTION("_ldrb_reg_16 R%d,[R%d,R%d]\n", Rt, Rn, Rm);
}

inline void decode_ldr_str_imm_16(uint16_t ins_code, _O uint32_t* imm, _O uint32_t* Rt, _O uint32_t* Rn)
{
	*Rt = LOW_BIT16(ins_code, 3);
	*Rn = LOW_BIT16(ins_code>>3, 3);
	*imm = LOW_BIT16(ins_code>>6, 5);
}

void _str_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t imm5, Rt, Rn;
	decode_ldr_str_imm_16(ins_code, &imm5, &Rt, &Rn);
	bool_t add = TRUE;
	bool_t index = TRUE;
	bool_t wback = FALSE;

	_str_imm(imm5, Rn, Rt, add, index, wback, regs, state, memory);
	LOG_INSTRUCTION("_str_imm_16 R%d,[R%d,#0x%d]\n", Rt, Rn, imm5);
}

void _ldr_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t imm5, Rt, Rn;
	decode_ldr_str_imm_16(ins_code, &imm5, &Rt, &Rn);
	bool_t add = TRUE;
	bool_t index = TRUE;
	bool_t wback = FALSE;

	_ldr_imm(imm5, Rn, Rt, add, index, wback, regs, state, memory);
	LOG_INSTRUCTION("_ldr_imm_16 R%d,[R%d,#0x%d]\n", Rt, Rn, imm5);
}

void _strb_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t imm5, Rt, Rn;
	decode_ldr_str_imm_16(ins_code, &imm5, &Rt, &Rn);
	bool_t add = TRUE;
	bool_t index = TRUE;
	bool_t wback = FALSE;

	_strb_imm(imm5, Rn, Rt, add, index, wback, regs, state, memory);
	LOG_INSTRUCTION("_strb_imm_16 R%d,[R%d,#0x%d]\n", Rt, Rn, imm5);
}

void _ldrb_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t imm5, Rt, Rn;
	decode_ldr_str_imm_16(ins_code, &imm5, &Rt, &Rn);
	bool_t add = TRUE;
	bool_t index = TRUE;
	bool_t wback = FALSE;

	_ldrb_imm(imm5, Rn, Rt, add, index, wback, regs, state, memory);
	LOG_INSTRUCTION("_ldrb_imm_16 R%d,[R%d,#0x%d]\n", Rt, Rn, imm5);
}

void _strh_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t imm5, Rt, Rn;
	decode_ldr_str_imm_16(ins_code, &imm5, &Rt, &Rn);
	bool_t add = TRUE;
	bool_t index = TRUE;
	bool_t wback = FALSE;

	_strh_imm(imm5, Rn, Rt, add, index, wback, regs, state, memory);
	LOG_INSTRUCTION("_strh_imm_16 R%d,[R%d,#0x%d]\n", Rt, Rn, imm5);
}

void _ldrh_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t imm5, Rt, Rn;
	decode_ldr_str_imm_16(ins_code, &imm5, &Rt, &Rn);
	bool_t add = TRUE;
	bool_t index = TRUE;
	bool_t wback = FALSE;

	_ldrh_imm(imm5, Rn, Rt, add, index, wback, regs, state, memory);
	LOG_INSTRUCTION("_ldrh_imm_16 R%d,[R%d,#0x%d]\n", Rt, Rn, imm5);
}

inline void decode_ldr_str_sp_imm_16(uint16_t ins_code, _O uint32_t* imm, _O uint32_t* Rt)
{
	*imm = LOW_BIT16(ins_code, 8);
	*Rt = LOW_BIT16(ins_code>>8, 3);
}

void _str_sp_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t imm8, Rt;
	uint32_t Rn = 13;
	decode_ldr_str_sp_imm_16(ins_code, &imm8, &Rt);

	bool_t index = TRUE;
	bool_t add = TRUE;
	bool_t wback = FALSE;

	_str_imm(imm8, Rn, Rt, add, index, wback, regs, state, memory);
	LOG_INSTRUCTION("_str_sp_imm_16 R%d,[SP,#0x%d]\n", Rt, imm8);
}

void _ldr_sp_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	armv7m_state* state = (armv7m_state*)run_info->cpu_spec_info;
	uint32_t imm8, Rt;
	uint32_t Rn = 13;
	decode_ldr_str_sp_imm_16(ins_code, &imm8, &Rt);

	bool_t index = TRUE;
	bool_t add = TRUE;
	bool_t wback = FALSE;

	_ldr_imm(imm8, Rn, Rt, add, index, wback, regs, state, memory);
	LOG_INSTRUCTION("_ldr_sp_imm_16 R%d,[SP,#0x%d]\n", Rt, imm8);
}

void _adr_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t imm8 = LOW_BIT16(ins_code, 8) << 2;
	uint32_t Rd = LOW_BIT16(ins_code>>8, 3);
	bool_t add = TRUE;

	_adr(imm8, Rd, add, regs);
	LOG_INSTRUCTION("_adr_16 R%d,#%d\n", Rd, imm8);
}

void _add_sp_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t imm8 = LOW_BIT16(ins_code, 8) << 2;
	uint32_t Rd = LOW_BIT16(ins_code>>8, 3);
	uint32_t setflags = FALSE;

	_add_sp_imm(imm8, Rd, setflags, regs);
	LOG_INSTRUCTION("_add_sp_imm_16 R%d,sp,#%d\n", Rd, imm8);
}

void _add_sp_sp_imm_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_reg_t* regs = ((armv7m_instruct_t*)run_info->ins_set)->regs;
	uint32_t imm7 = LOW_BIT16(ins_code, 7) << 2;
	uint32_t Rd = 13;
	uint32_t setflags = FALSE;

	_add_sp_imm(imm7, Rd, setflags, regs);
	LOG_INSTRUCTION("_add_sp_sp_imm_16 sp,sp,#%d\n", imm7);
}

void _unpredictable_16(uint16_t ins_code, run_info_t* run_info, memory_map_t* memory)
{
	LOG(LOG_WARN, "ins_code: %x is unpredictable, treated as NOP\n", ins_code);
}

/****** init instruct table ******/
void init_instruction_table(instruct_table_t* table)
{
	// 15~10 bit A5-156
	set_base_table_value(table->base_table16, 0x00, 0x0F, shift_add_sub_mov);		// 00xxxx		
	set_base_table_value(table->base_table16, 0x10, 0x10, data_process);			// 010000				
	set_base_table_value(table->base_table16, 0x11, 0x11, spdata_branch_exchange);	// 010001
	set_base_table_value(table->base_table16, 0x12, 0x13, load_literal_pool);		// 01001x
	set_base_table_value(table->base_table16, 0x14, 0x27, load_store_single);		// 0101xx 011xxx 100xxx
	set_base_table_value(table->base_table16, 0x28, 0x29, pc_related_address);		// 10100x
	set_base_table_value(table->base_table16, 0x2A, 0x2B, sp_related_address);		// 10101x
	set_base_table_value(table->base_table16, 0x2C, 0x2F, misc_16bit_ins);			// 1011xx
	//base_table[0x2C~0x2F] = misc_16bit_ins			// 1011xx
	//base_table[0x30~0x31] = store_multiple			// 11000x
	//base_table[0x32~0x33] = load_multiple			// 11001x
	//base_table[0x34~0x37] = con_branch_super_call	// 1101xx
	//base_table[0x38~0x39] = uncon_branch			// 11100x

	// shift(imm) add sub mov A5-157
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x00, 0x03, _lsl_imm_16);	// 000xx
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x04, 0x07, _lsr_imm_16);	// 001xx
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x08, 0x0B, _asr_imm_16);	// 010xx
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x0C, 0x0C, _add_reg_16);	// 01100
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x0D, 0x0D, _sub_reg_16);	// 01101
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x0E, 0x0E, _add_imm3_16);	// 01110
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x0F, 0x0F, _sub_imm3_16);	// 01111
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x10, 0x13, _mov_imm_16);	// 100xx
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x14, 0x17, _cmp_imm_16);	// 101xx
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x18, 0x1B, _add_imm8_16);	// 110xx
	set_sub_table_value(table->shift_add_sub_mov_table16, 0x1C, 0x1F, _sub_imm8_16);	// 111xx

	// data processing A5-158
	set_sub_table_value(table->data_process_table16, 0x00, 0x00, _and_reg_16);		// 0000
	set_sub_table_value(table->data_process_table16, 0x01, 0x01, _eor_reg_16);		// 0001
	set_sub_table_value(table->data_process_table16, 0x02, 0x02, _lsl_reg_16);		// 0010
	set_sub_table_value(table->data_process_table16, 0x03, 0x03, _lsr_reg_16);		// 0011
	set_sub_table_value(table->data_process_table16, 0x04, 0x04, _asr_reg_16);		// 0100
	set_sub_table_value(table->data_process_table16, 0x05, 0x05, _adc_reg_16);		// 0101
	set_sub_table_value(table->data_process_table16, 0x06, 0x06, _sbc_reg_16);		// 0110
	set_sub_table_value(table->data_process_table16, 0x07, 0x07, _ror_reg_16);		// 0111
	set_sub_table_value(table->data_process_table16, 0x08, 0x08, _tst_reg_16);		// 1000
	set_sub_table_value(table->data_process_table16, 0x09, 0x09, _rsb_imm_16);		// 1001
	set_sub_table_value(table->data_process_table16, 0x0A, 0x0A, _cmp_reg_16);		// 1010
	set_sub_table_value(table->data_process_table16, 0x0B, 0x0B, _cmn_reg_16);		// 1011
	set_sub_table_value(table->data_process_table16, 0x0C, 0x0C, _orr_reg_16);		// 1100
	set_sub_table_value(table->data_process_table16, 0x0D, 0x0D, _mul_reg_16);		// 1101
	set_sub_table_value(table->data_process_table16, 0x0E, 0x0E, _bic_reg_16);		// 1110
	set_sub_table_value(table->data_process_table16, 0x0F, 0x0F, _mvn_reg_16);		// 1111

	// special data instructions and branch and exchange A5-159
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x00, 0x03, _add_reg_spec_16);	// 00xx
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x04, 0x04, _unpredictable_16);	// 0100(*)
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x05, 0x07, _cmp_reg_spec_16);	// 0101,011x
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x08, 0x0B, _mov_reg_spec_16);	// 10xx
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x0C, 0x0D, _bx_spec_16);		// 110x
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x0E, 0x0F, _blx_spec_16);		// 111x

	// load from literal pool A7-289
	set_sub_table_value(table->load_literal_table16, 0x00, 0x00, _ldr_literal_16);

	// load store single data item A5-160
	set_sub_table_value(table->load_store_single_table16, 0x00, 0x27, _unpredictable_16);		// 0000 000 ~ 0010 011
	set_sub_table_value(table->load_store_single_table16, 0x28, 0x28, _str_reg_16);				// 0101 000
	set_sub_table_value(table->load_store_single_table16, 0x29, 0x29, _strh_reg_16);			// 0101 001
	set_sub_table_value(table->load_store_single_table16, 0x2A, 0x2A, _strb_reg_16);			// 0101 010
	set_sub_table_value(table->load_store_single_table16, 0x2B, 0x2B, _ldrsb_reg_16);			// 0101 011
	set_sub_table_value(table->load_store_single_table16, 0x2C, 0x2C, _ldr_reg_16);				// 0101	100
	set_sub_table_value(table->load_store_single_table16, 0x2D, 0x2D, _ldrh_reg_16);			// 0101 101
	set_sub_table_value(table->load_store_single_table16, 0x2E, 0x2E, _ldrb_reg_16);			// 0101 110
	set_sub_table_value(table->load_store_single_table16, 0x2F, 0x2F, _ldrsh_reg_16);			// 0101 111
	set_sub_table_value(table->load_store_single_table16, 0x2F, 0x2F, _ldrsh_reg_16);			// 0101 111
	set_sub_table_value(table->load_store_single_table16, 0x30, 0x33, _str_imm_16);				// 0110 0xx
	set_sub_table_value(table->load_store_single_table16, 0x34, 0x37, _ldr_imm_16);				// 0110 1xx
	set_sub_table_value(table->load_store_single_table16, 0x38, 0x3B, _strb_imm_16);			// 0111 0xx
	set_sub_table_value(table->load_store_single_table16, 0x3C, 0x3F, _ldrb_imm_16);			// 0111 1xx
	set_sub_table_value(table->load_store_single_table16, 0x40, 0x43, _strh_imm_16);			// 1000 0xx
	set_sub_table_value(table->load_store_single_table16, 0x44, 0x47, _ldrh_imm_16);			// 1000 1xx
	set_sub_table_value(table->load_store_single_table16, 0x48, 0x4B, _str_sp_imm_16);			// 1001 0xx
	set_sub_table_value(table->load_store_single_table16, 0x4C, 0x4F, _ldr_sp_imm_16);			// 1001 1xx

	// pc related address A7-229
	set_sub_table_value(table->pc_related_address_table16, 0x00, 0x00, _adr_16);

	// sp related address A7-225
	set_sub_table_value(table->sp_related_address_table16, 0x00, 0x00, _add_sp_imm_16);

	// misc 16-bit instructions A5-161
	set_sub_table_value(table->misc_16bit_ins_table, 0x00, 0x03, _add_sp_sp_imm_16);
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
void parse_opcode16(uint16_t opcode, run_info_t* run_info, memory_map_t* memory)
{
	armv7m_instruct_t* ins = (armv7m_instruct_t*)run_info->ins_set;
	ins->ins_table->base_table16[opcode >> 10](opcode, run_info, memory);
}

void parse_opcode32(uint32_t opcode, armv7m_instruct_t* ins)
{
//	ins->ins_table->base_table32[opcode >> 13](opcode, ins);
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

instruct_table_t* create_arm_v7m_ins_table()
{
	instruct_table_t* table = (instruct_table_t*)calloc(1, sizeof(instruct_table_t));
	if(table != NULL){
		init_instruction_table(table);
	}
	return table;
}

error_code_t destory_arm_v7m_ins_table(instruct_table_t** ptable)
{
	if(ptable == NULL || *ptable == NULL){
		return ERROR_NULL_POINTER;
	}

	free(*ptable);
	*ptable = NULL;

	return SUCCESS;
}


armv7m_instruct_t* create_armv7m_instruction()
{
	armv7m_instruct_t* ins = (armv7m_instruct_t*)calloc(1, sizeof(armv7m_instruct_t));
	if(ins != NULL){
		ins->regs = create_armv7m_regs();
		ins->ins_table = create_arm_v7m_ins_table();
	}

	return ins;
}

error_code_t ins_armv7m_init(cpu_t* cpu)
{
	armv7m_state* state = (armv7m_state* )malloc(sizeof(armv7m_state));
	if(state == NULL){
		return ERROR_NULL_POINTER;
	}
	set_cpu_spec_info(cpu, state);
	armv7m_instruct_t* ins = create_armv7m_instruction();
	set_cpu_ins(cpu, ins);

	return SUCCESS;
}

error_code_t destory_armv7m_instruction(armv7m_instruct_t** ins)
{
	if(ins == NULL || *ins == NULL){
		return ERROR_NULL_POINTER;
	}
	destory_arm_v7m_ins_table(&(*ins)->ins_table);
	destory_armv7m_regs(&(*ins)->regs);

	free(*ins);
	*ins = NULL;

	return SUCCESS;
}