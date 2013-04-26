#include "_types.h"
#include "arm_v7m_ins_decode.h"
#include <stdlib.h>

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
		printf("R%-3d=0x%x\n", i, regs->R[i]);
	}

	printf("MSP =0x%x\n", regs->MSP);
	printf("LR  =0x%x\n", regs->LR);
	printf("PC  =0x%x\n", regs->PC);
	printf("xPSR=0x%x\n\n", regs->xPSR);
}

/****** Here are some sub-parsing functions *******/
void shift_add_sub_mov(uint16_t opcode, armv7m_instruct_t* ins)
{
	// call appropriate instruction implement function by inquiring 13~9bit
	ins->ins_table->shift_add_sub_mov_table16[opcode >> 9 & 0x1F](opcode, ins->regs);
}

void data_process(uint16_t opcode, armv7m_instruct_t* ins)
{
	// call appropriate instruction implement function by inquiring 9~6bit
	ins->ins_table->data_process_table16[opcode >> 6 & 0x0F](opcode, ins->regs);	
}

void spdata_branch_exchange(uint16_t opcode, armv7m_instruct_t* ins)
{
	// call appropriate instruction implement function by inquiring 9~6bit
	ins->ins_table->spdata_branch_exchange_table16[opcode >> 6 & 0x0F](opcode, ins->regs);
}


uint32_t inITblock(armv7m_reg_t* regs)
{
						// bit 2 ~ 7				// bit 0 ~ 1
	uint8_t ITblock = (regs->xPSR >> 8 & 0xFC) | (regs->xPSR >> 25 & 0x3);
	return ITblock >> 5;
}

/****** Here are the final decoders of 16bit instructions ******/
void _lsl_imm_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	uint32_t imm = ins_code >> 6 & 0x1F;
	uint32_t Rm = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflag = not(inITblock(regs));

	_lsl_imm(imm, Rm, Rd, setflag, regs);
	LOG(LOG_INSTRUCTION, "_lsl_imm_16, R%d,R%d,#%d\n", Rd, Rm, imm);
}

void _lsr_imm_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	uint32_t imm = ins_code >> 6 & 0x1F;
	uint32_t Rm = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflag = not(inITblock(regs));
	
	_lsr_imm(imm, Rm, Rd, setflag, regs);		
	LOG(LOG_INSTRUCTION, "_lsr_imm_16, R%d,R%d,#%d\n", Rd, Rm, imm);
}

void _asr_imm_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	uint32_t imm = ins_code >> 6 & 0x1F;
	uint32_t Rm = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflag = not(inITblock(regs));

	_asr_imm(imm, Rm, Rd, setflag, regs);	
	LOG(LOG_INSTRUCTION, "_asr_imm_16, R%d,R%d,#%d\n", Rd, Rm, imm);
}

void _add_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	uint32_t Rm = ins_code >> 6 & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflag = not(inITblock(regs));

	_add_reg(Rm, Rn, Rd, SRType_LSL, 0, setflag, regs);
	LOG(LOG_INSTRUCTION, "_add_reg_16, R%d,R%d,R%d\n", Rd, Rn, Rm);	
}

void _sub_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	uint32_t Rm = ins_code >> 6 & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflag = not(inITblock(regs));

	_sub_reg(Rm, Rn, Rd, SRType_LSL, 0, setflag, regs);
	LOG(LOG_INSTRUCTION, "_sub_reg_16, R%d,R%d,R%d\n", Rd, Rn, Rm);	
}

void _add_imm3_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	uint32_t imm32 = ins_code >> 6 & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflag = not(inITblock(regs));

	_add_imm(imm32, Rn, Rd, setflag, regs);	
	LOG(LOG_INSTRUCTION, "_add_imm3_16, R%d,R%d,#%d\n", Rd, Rn, imm32);	
}

void _sub_imm3_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	uint32_t imm32 = ins_code >> 6 & 0x7;
	uint32_t Rn = ins_code >> 3 & 0x7;
	uint32_t Rd = ins_code & 0x7;
	uint32_t setflag = not(inITblock(regs));

	_sub_imm(imm32, Rn, Rd, setflag, regs);	
	LOG(LOG_INSTRUCTION, "_add_imm3_16, R%d,R%d,#%d\n", Rd, Rn, imm32);		
}

void _mov_imm_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	uint32_t d = ins_code >> 8 & 0x7;
	uint32_t imm = ins_code & 0x7;
	uint32_t setflag = not(inITblock(regs));
	uint32_t carry = GET_APSR_C(regs);

	_mov_imm(d, imm, setflag, carry, regs);
	LOG(LOG_INSTRUCTION, "_mov_imm_16, R%d,#%d\n", d, imm);
}

void _cmp_imm_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_cmp_imm_16\n");
}

void _add_imm8_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_add_imm8_16\n");
}

void _sub_imm8_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_add_imm8_16\n");
}

/************************/
void _and_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_and_reg_16\n");
}

void _eor_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_eor_reg_16\n");
}

void _lsl_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_lsl_reg_16\n");
}

void _lsr_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_lsr_reg_16\n");
}

void _asr_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_asr_reg_16\n");
}

void _adc_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_adc_reg_16\n");
}

void _sbc_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_sbc_reg_16\n");
}

void _ror_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_ror_reg_16\n");
}

void _tst_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_tst_reg_16\n");
}

void _rsb_imm_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_rsb_imm_16\n");
}

void _add_reg_spec_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_add_reg_spec_16\n");
}

void _cmp_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_cmp_reg_16\n");
}

void _cmn_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_cmn_reg_16\n");
}

void _orr_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_orr_reg_16\n");
}

void _mul_reg2_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_mul_reg2_16\n");
}

void _bic_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_bic_reg_16\n");
}

void _mvn_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_bic_reg_16\n");
}

void _mov_reg_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	
	uint32_t Rm = ins_code>> 3 & 0x3F;
	uint32_t Rdn = ins_code & 0x7;
	uint32_t Rd = Rdn | (ins_code >> 4) & 0x8L;
	uint32_t Rn = Rd;
	uint32_t setflag = 0;

	if(Rd == 0x0D){
		// SEE ADD SP PLUS REGISTER
	}else{
		_add_reg(Rm, Rn, Rd, SRType_LSL, 0, setflag, regs);
	}

	//UNPREDICTABLE
	LOG(LOG_INSTRUCTION, "_mov_reg_16, R%d,R%d\n", Rd, Rm);
}

void _bx_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_bx_16\n");
}

void _blx_16(uint16_t ins_code, armv7m_reg_t* regs)
{
	printf("_blx_16\n");
}





/****** init instruct table ******/
void init_instruction_table(instruct_table_t* table)
{
	// 15~10 bit A5-156
	set_base_table_value(table->base_table16, 0x00, 0x0F, shift_add_sub_mov);		// 00xxxx		
	set_base_table_value(table->base_table16, 0x10, 0x10, data_process);			// 010000				
	set_base_table_value(table->base_table16, 0x11, 0x11, spdata_branch_exchange);	// 010001
	//base_table[0x12~0x13] = load_literal			// 01001x
	//base_table[0x14~0x17] = load_store_single		// 0101xx
	//base_table[0x18~0x1F] = load_store_single		// 011xxx
	//base_table[0x20~0x27] = load_store_single		// 100xxx
	//base_table[0x28~0x29] = general_PC_address		// 10100x
	//base_table[0x2A~0x2B] = general_SP_address		// 10101x
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
	set_sub_table_value(table->data_process_table16, 0x0D, 0x0D, _mul_reg2_16);	// 1101
	set_sub_table_value(table->data_process_table16, 0x0E, 0x0E, _bic_reg_16);		// 1110
	set_sub_table_value(table->data_process_table16, 0x0F, 0x0F, _mvn_reg_16);		// 1111

	// special data instructions and branch and exchange A5-159
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x00, 0x03, _add_reg_spec_16);	// 00xx
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x05, 0x07, _cmp_reg_16);	// 0101,011x
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x08, 0x0B, _mov_reg_16);	// 10xx
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x0C, 0x0D, _bx_16);		// 110x
	set_sub_table_value(table->spdata_branch_exchange_table16, 0x0E, 0x0F, _blx_16);		// 111x
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
void parse_opcode16(uint16_t opcode, armv7m_instruct_t* ins)
{
	ins->ins_table->base_table16[opcode >> 10](opcode, ins);
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