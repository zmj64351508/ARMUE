#include "arm_v7m_ins_implement.h"
#include "error_code.h"

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

/*<<ARMv7-M Architecture Reference Manual A2-43>>*/
inline void AddWithCarry(uint32_t op1, uint32_t op2, uint32_t carry_in, uint32_t* result, uint32_t* carry_out, uint32_t *overflow)
{
	if(sizeof(uint64_t) == 8){
		uint64_t unsigned_sum = (uint64_t)op1 + (uint64_t)op2 + (uint64_t)carry_in;
		int64_t op1_64 = (int32_t)op1;
		int64_t op2_64 = (int32_t)op2;
		int64_t signed_sum = op1_64 + op2_64 + (int64_t)carry_in;
		uint32_t _result = unsigned_sum & 0xFFFFFFFFL;
		*overflow = (signed_sum == (int32_t)_result)? 0 : 1;
		*carry_out = (unsigned_sum == (uint64_t)_result) ? 0 : 1;
		*result = _result;
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
	(result, carry, overflow) = AddWithCarry(R[n], imm32, ¡®0¡¯);
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
	(result, carry, overflow) = AddWithCarry(R[n], NOT(imm32), ¡®1¡¯);
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
	(result, carry, overflow) = AddWithCarry(R[n], NOT(imm32), ¡®1¡¯);
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
void _mov_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs)
{
	uint32_t Rm_val = GET_REG_VAL(regs, Rm);
	uint32_t result = Rm_val;
	if(Rd == 15){

	}else{
		SET_REG_VAL(regs, Rd, result);
		if(setflags){
			SET_APSR_N(regs, result);
			SET_APSR_Z(regs, result);
		}
	}
}