#ifndef _ARM_V7M_INSTRUCTION_
#define _ARM_V7M_INSTRUCTION_

#include "error_code.h"
#include "arm_v7m_ins_implement.h"
#include "cpu.h"

enum ARM_INS_TYPE{
	ARM_INS_THUMB16,
	ARM_INS_THUMB32,
	ARM_INS_ARM,
};

#define LOW_BIT32(value, bit_num) (assert(bit_num < 32 && bit_num > 0), ((value) & (0xFFFFFFFF >> (32-(bit_num)))))
#define LOW_BIT16(value, bit_num) (assert(bit_num < 16 && bit_num > 0), ((value) & (0xFFFF >> (16-(bit_num)))))

// 16bit thumb-2 instruction type
struct armv7m_instruct_t;
typedef void (*_armv7m_translate16_t)(uint16_t opcode, struct cpu_t* cpu);
typedef _armv7m_translate16_t (*thumb_translate16_t)(uint16_t opcode, struct cpu_t* cpu);

typedef void (*_armv7m_translate32_t)(uint16_t opcode, struct cpu_t* cpu);
typedef _armv7m_translate32_t (*thumb_translate32_t)(uint16_t opcode, struct cpu_t* cpu);

#define THUMB_DECODER 1
#define THUMB_EXCUTER 2
typedef struct thumb_decode_t{
	union{
		thumb_translate16_t translater16;
		thumb_translate32_t translater32;
	};
	uint8_t type;
}thumb_decode_t;

#define BASE_TABLE_SIZE_16				64
#define SHIFT_ADD_SUB_MOV_SIZE_16		64
#define DATA_PROCESS_SIZE_16			32	
#define SPDATA_BRANCH_EXCHANGE_SIZE_16	16
#define LOAD_STORE_SINGLE_SIZE_16		128
#define MISC_16BIT_INS_SIZE				128
#define CON_BRANCH_SVC_SIZE_16			16

#define BASE_TABLE_SIZE_32				256

// all the classifictions are followed by the definition of "ARMv7-m Architecture Reference manual A5-156"
typedef struct {
	// base_table stores the sub-parsing functions pointer classified by 15~11bit of the instruction
	thumb_decode_t		base_table16[BASE_TABLE_SIZE_16];

	// all the sub-tables stores the appropriate implement function pointer of the instruction
	thumb_decode_t		shift_add_sub_mov_table16[SHIFT_ADD_SUB_MOV_SIZE_16];
	thumb_decode_t		data_process_table16[DATA_PROCESS_SIZE_16];
	thumb_decode_t		spdata_branch_exchange_table16[SPDATA_BRANCH_EXCHANGE_SIZE_16];
	thumb_decode_t		load_store_single_table16[LOAD_STORE_SINGLE_SIZE_16];
	thumb_decode_t		misc_16bit_ins_table16[MISC_16BIT_INS_SIZE];
	thumb_decode_t		con_branch_svc_table16[CON_BRANCH_SVC_SIZE_16];

	thumb_decode_t		base_table32[BASE_TABLE_SIZE_32];
}thumb_instruct_table_t;

void armv7m_print_reg_val(armv7m_reg_t* regs);

bool_t is_16bit_code(uint16_t opcode);
uint32_t align_address(uint32_t address);
thumb_translate16_t thumb_parse_opcode16(uint16_t opcode, cpu_t* cpu);
void armv7m_next_PC(cpu_t* cpu, int ins_length);
int armv7m_PC_modified(cpu_t* cpu);
error_code_t ins_thumb_destory(cpu_t* cpu);
error_code_t ins_thumb_init(cpu_t* cpu);
#endif