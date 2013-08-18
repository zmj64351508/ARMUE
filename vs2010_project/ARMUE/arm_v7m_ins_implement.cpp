#include "arm_v7m_ins_implement.h"
#include "error_code.h"
#include <assert.h>

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
// MS VC will ASR signed int
#ifdef _MSC_VER	
	*result = (int32_t)val >> shift;
#else
#error Unknow compiler
#endif
}

void ROR_C(uint32_t val, int shift, _O uint32_t* result, _O int *carry_out)
{
	assert(shift != 0);
	*carry_out = val >> (shift - 1) & BIT_0;
	uint32_t shift_out = val << (32-shift);
	*result = val >> shift | shift_out;
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

/* <<ARMv7-M Architecture Reference Manual 630>> */
inline void BranchTo(uint32_t address, armv7m_reg_t* regs)
{
	SET_REG_VAL(regs, PC_index, address);
}

/* <<ARMv7-M Architecture Reference Manual 47>> */
inline void BranchWritePC(uint32_t address, armv7m_reg_t* regs)
{
	BranchTo(DOWN_ALIGN(address, 1), regs);
}

/* <<ARMv7-M Architecture Reference Manual 48>> */
inline void ALUWritePC(uint32_t address, armv7m_reg_t* regs)
{
	BranchWritePC(address, regs);
}


/* <<ARMv7-M Architecture Reference Manual 47>> */
void BXWritePC(uint32_t address, armv7m_reg_t* regs)
{
	uint32_t Tflag;
	int CurrentMode = regs->mode;
	if(CurrentMode == MODE_HANDLER &&
		((address >> 28) & 0xFul) == 0xFul){
			//TODO: ExceptionReturn();
	}else{
		Tflag = address & 0x1ul;
		SET_EPSR_T(regs, Tflag);
		if(Tflag == 0){
			//TODO: UsageFault(Invalid_State);
		}

		BranchTo(DOWN_ALIGN(address, 1), regs);
	}
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
void _lsl_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
	int carry = 0;
	uint32_t result;
	
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
void _lsr_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
	int carry = 0;
	uint32_t result;

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
void _asr_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{

	int carry = 0;
	uint32_t result;

	Shift_C(GET_REG_VAL(regs, Rm), SRType_ASR, imm, GET_APSR_C(regs), &result, &carry);
	SET_REG_VAL(regs, Rd, result);

	if(setflags != 0){
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
void _add_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs)
{
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
	(result, carry, overflow) = AddWithCarry(SP, shifted, ��0��);
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
void _add_sp_reg(uint32_t Rm, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs)
{
	uint32_t carry = GET_APSR_C(regs);
	uint32_t overflow;
	uint32_t shifted;
	Shift(GET_REG_VAL(regs, Rm), shift_t, shift_n, carry, &shifted);
	
	uint32_t SP_val = GET_REG_VAL(regs, SP_index);
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
void _sub_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs)
{
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
	(result, carry, overflow) = AddWithCarry(R[n], imm32, ��0��);
	R[d] = result;
	if setflags then
		APSR.N = result<31>;
		APSR.Z = IsZeroBit(result);
		APSR.C = carry;
		APSR.V = overflow;
*************************************/
void _add_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
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
	(result, carry, overflow) = AddWithCarry(R[n], NOT(imm32), ��1��);
	R[d] = result;
	if setflags then
		APSR.N = result<31>;
		APSR.Z = IsZeroBit(result);
		APSR.C = carry;
		APSR.V = overflow;
*************************************/
void _sub_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
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
void _mov_imm(uint32_t Rd, uint32_t imm32, uint32_t setflags, int carry, armv7m_reg_t* regs)
{
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
	(result, carry, overflow) = AddWithCarry(R[n], NOT(imm32), ��1��);
	APSR.N = result<31>;
	APSR.Z = IsZeroBit(result);
	APSR.C = carry;
	APSR.V = overflow;
*************************************/
void _cmp_imm(uint32_t imm32, uint32_t Rn, armv7m_reg_t* regs)
{
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
void _and_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs)
{
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
*********o****************************/
void _eor_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs)
{
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
*********o****************************/
void _lsl_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
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
*********o****************************/
void _lsr_reg(uint32_t Rm, uint32_t Rn ,uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
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
*********o****************************/
void _asr_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
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
*********o****************************/
void _adc_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs)
{
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
*********o****************************/
void _sbc_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs)
{
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
*********o****************************/
void _ror_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
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
*********o****************************/
void _tst_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, armv7m_reg_t* regs)
{
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
<<ARMv7-M Architecture Reference Manual A7-411>>
if ConditionPassed() then
	EncodingSpecificOperations();
	(result, carry, overflow) = AddWithCarry(NOT(R[n]), imm32, ��1��);
	R[d] = result;
	if setflags then
		APSR.N = result<31>;
		APSR.Z = IsZeroBit(result);
		APSR.C = carry;
		APSR.V = overflow;
*********o****************************/
void _rsb_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
	/* Warning: this function is not tested since MDK can't generate 16bit code for it */
	uint32_t result;
	uint32_t overflow;
	uint32_t carry = GET_APSR_C(regs);
	uint32_t Rn_val = GET_REG_VAL(regs, Rn);
	AddWithCarry(~Rn_val, imm32, carry, &result, &carry, &overflow);
	SET_REG_VAL(regs, Rd, result);
	if(setflags){
		SET_APSR_N(regs, result);
		SET_APSR_Z(regs, result);
		SET_APSR_C(regs, carry);
		SET_APSR_V(regs, overflow);
	}
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-263>>
if ConditionPassed() then
	EncodingSpecificOperations();
	shifted = Shift(R[m], shift_t, shift_n, APSR.C);
	(result, carry, overflow) = AddWithCarry(R[n], NOT(shifted), ��1��);
	APSR.N = result<31>;
	APSR.Z = IsZeroBit(result);
	APSR.C = carry;
	APSR.V = overflow;
*********o****************************/
void _cmp_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, armv7m_reg_t* regs)
{
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
	(result, carry, overflow) = AddWithCarry(R[n], shifted, ��0��);
	APSR.N = result<31>;
	APSR.Z = IsZeroBit(result);
	APSR.C = carry;
	APSR.V = overflow;
*********o****************************/
void _cmn_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, armv7m_reg_t* regs)
{
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
*********o****************************/
void _orr_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs)
{
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
*********o****************************/
void _mul_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
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
*********o****************************/
void _bic_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs)
{
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
*********o****************************/
void _mvn_reg(uint32_t Rm, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs)
{
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
*********o****************************/
void _mov_reg(uint32_t Rm, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
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
<<ARMv7-M Architecture Reference Manual A7-250>>
if ConditionPassed() then
	EncodingSpecificOperations();
	BXWritePC(R[m]);
*********o****************************/
void _bx(uint32_t Rm, armv7m_reg_t* regs)
{
	uint32_t Rm_val = GET_REG_VAL(regs, Rm);
	BXWritePC(Rm_val, regs);
}

/***********************************
<<ARMv7-M Architecture Reference Manual A7-249>>
if ConditionPassed() then
	EncodingSpecificOperations();
	target = R[m];
	next_instr_addr = PC - 2;
	LR = next_instr_addr<31:1> : ��1��;
	BLXWritePC(target);
*********o****************************/
void _blx(uint32_t Rm, armv7m_reg_t* regs)
{
	uint32_t target = GET_REG_VAL(regs, Rm);
	uint32_t PC_val = GET_REG_VAL(regs, PC_index);
	uint32_t next_instr_addr = PC_val - 2;
	uint32_t LR_val = next_instr_addr | 0x1ul;
	SET_REG_VAL(regs, LR_index, LR_val);
	BXWritePC(target, regs);
}