#ifndef _ARM_V7M_INS_IMPLEMENT_H_ 
#define _ARM_V7M_INS_IMPLEMENT_H_

#include "_types.h"

typedef enum{
	SRType_LSL,
	SRType_LSR,
	SRType_ASR,
	SRType_ROR,
	SRType_RRX,
}SRType;

#define MODE_THREAD		1
#define MODE_HANDLER	2

#define SP_index 13
#define LR_index 14
#define PC_index 15

typedef struct{
	// general registers
	uint32_t R[16];			// 0x00~0x0D * sizeof(uint32_t)

	#define MSP R[13]
	#define PSP R[13]
	#define LR R[14]
	#define PC R[15]

	// control registers
	uint32_t xPSR;			// 0x11
	uint32_t PRIMASK;		// 0x12
	uint32_t FAULTMASK;		// 0x13
	uint32_t BASEPRI;		// 0x14
	uint32_t CONTROL;		// 0x15
	uint32_t SP_back;		// backup for MSP/PSP
	uint32_t PC_return;		/* the return value of PC, it is the same as PC in 32-bit instructions
							   and PC+2 in 16-bit instructions */

	int mode;
}armv7m_reg_t;


#define PSR_N (0x1L << 31)
#define PSR_Z (0x1L << 30)
#define PSR_C (0x1L << 29)
#define PSR_V (0x1L << 28)
#define PSR_T (0x1L << 24)

#define BIT_31 (0x1L << 31)
#define BIT_1	(0x1L << 1)
#define BIT_0	(0x1L)

#define SET_APSR_N(regs, result_reg) set_bit(&(regs)->xPSR, PSR_N, (result_reg) & BIT_31)
#define SET_APSR_Z(regs, result_reg) set_bit(&(regs)->xPSR, PSR_Z, (result_reg) == 0 ? 1 : 0)
#define SET_APSR_C(regs, carry) set_bit(&(regs)->xPSR, PSR_C, (carry))
#define SET_APSR_V(regs, overflow) set_bit(&(regs)->xPSR, PSR_V, (overflow))
#define SET_EPSR_T(regs, bit) set_bit(&(regs)->xPSR, PSR_T, (bit));

#define GET_APSR_C(regs) get_bit(&(regs)->xPSR, PSR_C)

#define GET_REG_VAL_UNCON(regs, Rx) (*(&(regs)->R[0]+(Rx)))
#define SET_REG_VAL_UNCON(regs, Rx, val) (*(&(regs)->R[0]+(Rx)) = (val))

#define DOWN_ALIGN(val, order) ((val) & (0xFFFFFFFFUL << order))
#define CHECK_PC(PC_val) ((PC_val) & 0x1ul)

inline uint32_t GET_REG_VAL(armv7m_reg_t* regs, uint32_t Rx){
	uint32_t val = 0;
	/* PC is special, it always return the value aligned to 4*/
	if(Rx == PC_index){
		val = regs->PC_return;
	}else{
		val = GET_REG_VAL_UNCON(regs, Rx);
	}
	return val;
}

inline void SET_REG_VAL(armv7m_reg_t* regs, uint32_t Rx, uint32_t val){
	uint32_t modified = val;
	switch(Rx){
	case SP_index:
		modified = DOWN_ALIGN(modified, 2);
		break;
	default:
		break;
	}
	SET_REG_VAL_UNCON(regs, Rx, modified);
}


inline uint8_t get_bit(uint32_t* reg, uint32_t bit_pos)
{
	return (*reg & bit_pos) == 0 ? 0: 1;
}

// if bit_val != 0 then set bit to 1. bit_pos is defined above
inline void set_bit(uint32_t* reg, uint32_t bit_pos,int bit_val)
{
	if(bit_val == 0){
		*reg &= ~bit_pos;
	}else{
		*reg |= bit_pos;
	}
}

void _lsl_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs);
void _lsr_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs);
void _asr_imm(uint32_t imm, uint32_t Rm, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs);
void _add_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs);
void _add_sp_reg(uint32_t Rm, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs);
void _sub_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs);
void _add_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs);
void _sub_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs);
void _mov_imm(uint32_t d, uint32_t imm32, uint32_t setflag, int carry, armv7m_reg_t* regs);
void _cmp_imm(uint32_t imm32, uint32_t Rn, armv7m_reg_t* regs);
void _and_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs);
void _eor_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs);
void _lsl_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflag, armv7m_reg_t* regs);
void _lsr_reg(uint32_t Rm, uint32_t Rn ,uint32_t Rd, uint32_t setflag, armv7m_reg_t* regs);
void _asr_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflag, armv7m_reg_t* regs);
void _adc_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs);
void _sbc_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs);
void _ror_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs);
void _tst_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, armv7m_reg_t* regs);
void _rsb_imm(uint32_t imm32, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs);
void _cmp_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, armv7m_reg_t* regs);
void _cmn_reg(uint32_t Rm, uint32_t Rn, SRType shift_t, uint32_t shift_n, armv7m_reg_t* regs);
void _orr_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs);
void _mul_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, uint32_t setflags, armv7m_reg_t* regs);
void _bic_reg(uint32_t Rm, uint32_t Rn, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs);
void _mvn_reg(uint32_t Rm, uint32_t Rd, SRType shift_t, uint32_t shift_n, uint32_t setflags, armv7m_reg_t* regs);
void _mov_reg(uint32_t Rm, uint32_t Rd, uint32_t setflag, armv7m_reg_t* regs);
void _bx(uint32_t Rm, armv7m_reg_t* regs);
void _blx(uint32_t Rm, armv7m_reg_t* regs);
#endif
