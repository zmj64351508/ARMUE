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


typedef struct{
	// general registers
	uint32_t R[13];			// 0x00~0x0D * sizeof(uint32_t)
	union{					// 0x0E
		uint32_t MSP;		
		uint32_t PSP;
	};
	uint32_t LR;			// 0x0F
	uint32_t PC;			// 0x10

	// control registers
	uint32_t xPSR;			// 0x11
	uint32_t PRIMASK;		// 0x12
	uint32_t FAULTMASK;		// 0x13
	uint32_t BASEPRI;		// 0x14
	uint32_t CONTROL;		// 0x15
}armv7m_reg_t;


#define N (0x1L << 31)
#define Z (0x1L << 30)
#define C (0x1L << 29)
#define V (0x1L << 28)

#define BIT_31 (0x1L << 31)
#define BIT_1	(0x1L << 1)
#define BIT_0	(0x1L)

#define SET_APSR_N(regs, result_reg) set_bit(&(regs)->xPSR, N, (result_reg) & BIT_31)
#define SET_APSR_Z(regs, result_reg) set_bit(&(regs)->xPSR, Z, (result_reg) == 0 ? 1 : 0)
#define SET_APSR_C(regs, carry) set_bit(&(regs)->xPSR, C, (carry))
#define SET_APSR_V(regs, overflow) set_bit(&(regs)->xPSR, V, (overflow))

#define GET_APSR_C(regs) get_bit(&(regs)->xPSR, C)

#define GET_REG_VAL(regs, Rx) (*(&(regs)->R[0]+(Rx)))
#define SET_REG_VAL(regs, Rx, val) (*(&(regs)->R[0]+(Rx)) = (val))

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
#endif
