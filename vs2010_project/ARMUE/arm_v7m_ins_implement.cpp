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

inline int check_add_overflow(uint32_t op1, uint32_t op2, uint32_t result)
{
	if((op1 & BIT_31) == (op2 & BIT_31) && 
		(op2 & BIT_31) != (result & BIT_31)){
			return 1;
	}else{
		return 0;
	}
}

// add result = op1 + op2
inline int check_add_carry(uint32_t op1, uint32_t op2, uint32_t result)
{
	if(result < op1 && result < op2){
		return 1;
	}else{
		return 0;
	}
}

// minus result = op1 + ~minuend + 1 , so the carry should be calculated by these 3 numbers
inline int check_minus_carry(uint32_t op1, uint32_t minuend, uint32_t result){
	int carry;
	// check op1 + ~minuend
	carry = check_add_carry(op1, ~minuend, op1+~minuend);
	if(carry == 1){
		return carry;
	}
	// check op1 + ~minuend + 1
	carry = check_add_carry(op1+~minuend, 1, result);
	return carry;
	
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
	
	Shift_C(*(&regs->R[0]+Rm), SRType_LSL, imm, GET_APSR_C(regs), &result, &carry);
	*(&regs->R[0]+Rd) = result;

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

	Shift_C(*(&regs->R[0]+Rm), SRType_LSR, imm, GET_APSR_C(regs), &result, &carry);
	*(&regs->R[0]+Rd) = result;

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

	Shift_C(*(&regs->R[0]+Rm), SRType_ASR, imm, GET_APSR_C(regs), &result, &carry);
	*(&regs->R[0]+Rd) = result;

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
	int carry = GET_APSR_C(regs);;
	int overflow = 0;
	
	// 
	uint32_t shifted;
	Shift(*(&regs->R[0]+Rm), shift_t, shift_n, carry, &shifted);
	
	uint32_t Rn_val = *(&regs->R[0]+Rn);
	*(&regs->R[0]+Rd) =  Rn_val + shifted;
	
	// software won't distinguish PC and other registers like hardware did. Just disable setflags.
	if(Rd == 15){
		setflags = 0;
	}
	uint32_t result = *(&regs->R[0]+Rd);
	
	// if Rn and shifed have the same sign, and Rd haven't..then overflow = 1
	overflow = check_add_overflow(Rn_val, shifted, result);

	// if Rd < Rn and shifted then carry = 1	
	carry = check_add_carry(Rn_val, shifted, result);
	
	if(setflags != 0){
		SET_APSR_N(regs, result);
		SET_APSR_Z(regs, result);
		SET_APSR_C(regs, carry);
		SET_APSR_V(regs, overflow);
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
	overflow = check_add_overflow(Rn_val, (uint32_t)-(int32_t)shifted, Rd_val);

	// if Rd < Rn and shifted then carry = 1	
	carry = check_minus_carry(Rn_val, shifted, Rd_val);

	if(setflags != 0){
		SET_APSR_N(regs, *(&regs->R[0]+Rd));
		SET_APSR_Z(regs, *(&regs->R[0]+Rd));
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
	int carry = GET_APSR_C(regs);
	int overflow = 0;

	uint32_t Rn_val = *(&regs->R[0]+Rn);
	*(&regs->R[0]+Rd) =  Rn_val + imm32;	
	uint32_t Rd_val = *(&regs->R[0]+Rd);

	// if Rn and shifed have the same sign, and Rd haven't..then overflow = 1
	overflow = check_add_overflow(Rn_val, imm32, Rd_val);

	// if Rd < Rn and shifted then carry = 1	
	carry = check_add_carry(Rn_val, imm32, Rd_val);

	if(setflags != 0){
		SET_APSR_N(regs, *(&regs->R[0]+Rd));
		SET_APSR_Z(regs, *(&regs->R[0]+Rd));
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
	int carry = GET_APSR_C(regs);
	int overflow = 0;

	uint32_t Rn_val = *(&regs->R[0]+Rn);
	*(&regs->R[0]+Rd) =  Rn_val - imm32;
	uint32_t Rd_val = *(&regs->R[0]+Rd);

	// if Rn and shifed have the same sign, and Rd haven't..then overflow = 1
	overflow = check_add_overflow(Rn_val, (uint32_t)-(int32_t)imm32, Rd_val);

	// if Rd < Rn and shifted then carry = 1	
	carry = check_minus_carry(Rn_val, imm32, Rd_val);

	if(setflags != 0){
		SET_APSR_N(regs, *(&regs->R[0]+Rd));
		SET_APSR_Z(regs, *(&regs->R[0]+Rd));
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
void _mov_imm(uint32_t Rd, uint32_t imm32, uint32_t setflag, int carry, armv7m_reg_t* regs)
{
	uint32_t result = imm32;
	*(&regs->R[0]+Rd) = result;
	if(setflag != 0){
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
	int carry = GET_APSR_C(regs);
	int overflow = 0;
	uint32_t result;

	uint32_t Rn_val = *(&regs->R[0]+Rn);
	result =  Rn_val - imm32;

	// if Rn and imm have the same sign, and Rd haven't..then overflow = 1
	overflow = check_add_overflow(Rn_val, (uint32_t)-(int32_t)imm32, result);

	// if Rd < Rn and imm then carry = 1			
	carry = check_minus_carry(Rn_val, imm32, result);

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
void _and_reg(uint32_t Rm, uint32_t Rdn, uint32_t setflag, armv7m_reg_t* regs)
{
	int carry = GET_APSR_C(regs);

	uint32_t shifted;
	Shift_C(*(&regs->R[0]+Rm), SRType_LSL, 0, carry, &shifted, &carry);

	uint32_t Rn_val = *(&regs->R[0]+Rdn);
	uint32_t result = Rn_val & shifted;
	*(&regs->R[0]+Rdn) = result;
	if(setflag){
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
void _eor_reg(uint32_t Rm, uint32_t Rdn, uint32_t setflag, armv7m_reg_t* regs)
{
	int carry = GET_APSR_C(regs);
	uint32_t shifted;
	Shift_C(*(&regs->R[0]+Rm), SRType_LSL, 0, carry, &shifted, &carry);
	uint32_t Rn_val = *(&regs->R[0]+Rdn);
	uint32_t result = Rn_val^shifted;
	*(&regs->R[0]+Rdn) = result;
	if(setflag){
		SET_APSR_N(regs, result);
		SET_APSR_Z(regs, result);
		SET_APSR_C(regs, carry);
	}
}

/***********************************
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
void _lsl_reg(uint32_t Rm, uint32_t Rdn, uint32_t setflag, armv7m_reg_t* regs)
{
	uint32_t shift_n = *(&regs->R[0]+Rm) & 0x1F;
	uint32_t result;
	uint32_t Rn_val = *(&regs->R[0]+Rdn);
	int carry = GET_APSR_C(regs);
	Shift_C(Rn_val, SRType_LSL, shift_n, carry, &result, &carry);
	*(&regs->R[0]+Rdn) = result;

	if(setflag){
		SET_APSR_N(regs, result);
		SET_APSR_Z(regs, result);
		SET_APSR_C(regs, carry);
	}
}