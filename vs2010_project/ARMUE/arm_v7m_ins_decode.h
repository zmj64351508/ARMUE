#ifndef _ARM_V7M_INSTRUCTION_
#define _ARM_V7M_INSTRUCTION_

#include "error_code.h"
#include "arm_v7m_ins_implement.h"
#include "cpu.h"


#define LOW_BIT32(value, bit_num) (assert(bit_num < 32 && bit_num > 0), ((value) & (0xFFFFFFFF >> (32-(bit_num)))))
#define LOW_BIT16(value, bit_num) (assert(bit_num < 16 && bit_num > 0), ((value) & (0xFFFF >> (16-(bit_num)))))

// 16bit thumb-2 instruction type
struct armv7m_instruct_t;
typedef void (*sub_translate16_t)(uint16_t opcode, struct run_info_t* run_info, memory_map_t* memory);
typedef void (*instruction16_t)(uint16_t opcode, struct run_info_t* run_info, memory_map_t* memory);


#define BASE_TABLE_SIZE_16				64
#define SHIFT_ADD_SUB_MOV_SIZE_16		64
#define DATA_PROCESS_SIZE_16			32	
#define SPDATA_BRANCH_EXCHANGE_SIZE_16	16
#define LOAD_LITERAL_SIZE_16			1
#define LOAD_STORE_SINGLE_SIZE_16		128
#define PC_RELATED_ADDRESS_SIZE_16		1
#define SP_RELATED_ADDRESS_SIZE_16		1
#define MISC_16BIT_INS_SIZE				128

// all the classifictions are followed by the definition of "ARMv7-m Architecture Reference manual A5-156"
typedef struct {
	// base_table stores the sub-parsing functions pointer classified by 15~11bit of the instruction
	sub_translate16_t	base_table16[BASE_TABLE_SIZE_16];

	// all the sub-tables stores the appropriate implement function pointer of the instruction
	instruction16_t		shift_add_sub_mov_table16[SHIFT_ADD_SUB_MOV_SIZE_16];
	instruction16_t		data_process_table16[DATA_PROCESS_SIZE_16];
	instruction16_t		spdata_branch_exchange_table16[SPDATA_BRANCH_EXCHANGE_SIZE_16];
	instruction16_t		load_literal_table16[LOAD_LITERAL_SIZE_16];
	instruction16_t		load_store_single_table16[LOAD_STORE_SINGLE_SIZE_16];
	instruction16_t		pc_related_address_table16[PC_RELATED_ADDRESS_SIZE_16];
	instruction16_t		sp_related_address_table16[SP_RELATED_ADDRESS_SIZE_16];
	instruction16_t		misc_16bit_ins_table[MISC_16BIT_INS_SIZE];
}instruct_table_t;

typedef struct armv7m_instruct_t{
	armv7m_reg_t* regs;
	instruct_table_t* ins_table;
}armv7m_instruct_t;

void print_reg_val(armv7m_reg_t* regs);

armv7m_instruct_t* create_armv7m_instruction();
error_code_t destory_armv7m_instruction(armv7m_instruct_t** ins);

bool_t is_16bit_code(uint16_t opcode);
uint32_t align_address(uint32_t address);
void parse_opcode16(uint16_t opcode16, run_info_t* run_info, memory_map_t* memory);
void init_instruction_table(instruct_table_t* table);
error_code_t ins_armv7m_init(cpu_t* cpu);
#endif