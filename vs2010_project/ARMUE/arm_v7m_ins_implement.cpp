#include "arm_v7m_ins_implement.h"

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
#ifdef _MSC_VER	
	*result = (int32_t)val >> shift;
#else
#error Unknow compiler
#endif
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
		break;
	case SRType_RRX:
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

inline int check_overflow(uint32_t op1, uint32_t op2, uint32_t result)
{
	if((op1 & BIT_31) == (op2 & BIT_31) && 
		(op2 & BIT_31) != (result & BIT_31)){
			return 1;
	}else{
		return 0;
	}
}

inline int check_add_carry(uint32_t op1, uint32_t op2, uint32_t result)
{
	if(result < op1 && result < op2){
		return 1;
	}else{
		return 0;
	}
}

void _lsl_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
	int carry = 0;
	uint32_t result;
	
	Shift_C(*(&regs->R[0]+Rm), SRType_LSL, imm, GET_APSR_C(regs), &result, &carry);
	*(&regs->R[0]+Rd) = result;

	if(setflags != 0){
		SET_APSR_N(result);
		SET_APSR_Z(result);
		SET_APSR_C(carry);
	}
}

void _lsr_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
	int carry = 0;
	uint32_t result;

	Shift_C(*(&regs->R[0]+Rm), SRType_LSR, imm, GET_APSR_C(regs), &result, &carry);
	*(&regs->R[0]+Rd) = result;

	if(setflags != 0){
		SET_APSR_N(result);
		SET_APSR_Z(result);
		SET_APSR_C(carry);	
	}
}

void _asr_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{

	int carry = 0;
	uint32_t result;

	Shift_C(*(&regs->R[0]+Rm), SRType_ASR, imm, GET_APSR_C(regs), &result, &carry);
	*(&regs->R[0]+Rd) = result;

	if(setflags != 0){
		SET_APSR_N(result);
		SET_APSR_Z(result);
		SET_APSR_C(carry);
	}
}

void _add_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs)
{
	int carry = GET_APSR_C(regs);;
	int overflow = 0;
	
	// 
	uint32_t shifted;
	Shift(*(&regs->R[0]+Rm), shift_t, shift_n, carry, &shifted);
	
	uint32_t Rn_val = *(&regs->R[0]+Rn);
	*(&regs->R[0]+Rd) =  Rn_val + shifted;
	uint32_t Rd_val = *(&regs->R[0]+Rd);
	
	// if Rn and shifed have the same sign, and Rd haven't..then overflow = 1
	overflow = check_overflow(Rn_val, shifted, Rd_val);

	// if Rd < Rn and shifted then carry = 1	
	carry = check_add_carry(Rn_val, shifted, Rd_val);
	
	if(setflags != 0){
		SET_APSR_N(*(&regs->R[0]+Rd));
		SET_APSR_Z(*(&regs->R[0]+Rd));
		SET_APSR_C(carry);
		SET_APSR_V(overflow);
	}
}

void _sub_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs)
{
	int carry = GET_APSR_C(regs);;
	int overflow = 0;

	// 
	uint32_t shifted;
	Shift(*(&regs->R[0]+Rm), shift_t, shift_n, carry, &shifted);
	shifted = 0-(int32_t)shifted;

	uint32_t Rn_val = *(&regs->R[0]+Rn);
	*(&regs->R[0]+Rd) =  Rn_val + shifted;
	uint32_t Rd_val = *(&regs->R[0]+Rd);

	// if Rn and shifed have the same sign, and Rd haven't..then overflow = 1
	overflow = check_overflow(Rn_val, (uint32_t)-(int32_t)shifted, Rd_val);

	// if Rd < Rn and shifted then carry = 1	
	carry = check_add_carry(Rn_val, (uint32_t)-(int32_t)shifted, Rd_val);

	if(setflags != 0){
		SET_APSR_N(*(&regs->R[0]+Rd));
		SET_APSR_Z(*(&regs->R[0]+Rd));
		SET_APSR_C(carry);
		SET_APSR_V(overflow);
	}
}

void _add_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
	int carry = GET_APSR_C(regs);
	int overflow = 0;

	uint32_t Rn_val = *(&regs->R[0]+Rn);
	*(&regs->R[0]+Rd) =  Rn_val + imm32;	
	uint32_t Rd_val = *(&regs->R[0]+Rd);

	// if Rn and shifed have the same sign, and Rd haven't..then overflow = 1
	overflow = check_overflow(Rn_val, imm32, Rd_val);

	// if Rd < Rn and shifted then carry = 1	
	carry = check_add_carry(Rn_val, imm32, Rd_val);

	if(setflags != 0){
		SET_APSR_N(*(&regs->R[0]+Rd));
		SET_APSR_Z(*(&regs->R[0]+Rd));
		SET_APSR_C(carry);
		SET_APSR_V(overflow);
	}
}

void _sub_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
	int carry = GET_APSR_C(regs);
	int overflow = 0;

	uint32_t Rn_val = *(&regs->R[0]+Rn);
	*(&regs->R[0]+Rd) =  Rn_val - imm32;
	uint32_t Rd_val = *(&regs->R[0]+Rd);

	// if Rn and shifed have the same sign, and Rd haven't..then overflow = 1
	overflow = check_overflow(Rn_val, (uint32_t)-(int32_t)imm32, Rd_val);

	// if Rd < Rn and shifted then carry = 1	
	carry = check_add_carry(Rn_val, (uint32_t)-(int32_t)imm32, Rd_val);

	if(setflags != 0){
		SET_APSR_N(*(&regs->R[0]+Rd));
		SET_APSR_Z(*(&regs->R[0]+Rd));
		SET_APSR_C(carry);
		SET_APSR_V(overflow);
	}
}

void _mov_imm(uint32_t Rd, uint32_t imm32, uint32_t setflag, int carry, armv7m_reg_t* regs)
{
	uint32_t result = imm32;
	*(&regs->R[0]+Rd) = result;
	if(setflag != 0){
		SET_APSR_N(result);
		SET_APSR_Z(result);
		SET_APSR_C(carry);
	}
}
