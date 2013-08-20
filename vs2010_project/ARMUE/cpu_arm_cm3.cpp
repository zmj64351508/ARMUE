#include "cpu.h"
#include "module_helper.h"
#include "error_code.h"
#include <stdlib.h>
#include "_types.h"
#include "arm_v7m_ins_decode.h"

static module_t* this_module;
static int registered = 0;

#include <stdio.h>

error_code_t init_armcm3_interrput_table(cpu_t* cpu)
{
	if(cpu == NULL || cpu->memory_map == NULL){
		return ERROR_NULL_POINTER;
	}

	memory_map_t* memory_map = cpu->memory_map;

	// search for start rom which base address is 0x00
	rom_t* start_rom = NULL;	
	for(int i = 0; i < ROM_MAX; i++){
		if(memory_map->ROM[i]->base_address == 0 && 
			memory_map->ROM[i]->allocated == TRUE){
				start_rom = memory_map->ROM[i];
				break;
		}
	}

	if(start_rom == NULL){
		return ERROR_NO_START_ROM;
	}

	uint32_t addr = 0;
	uint32_t interrput_vector = 0;
	int inpterrupt_table_size = sizeof(memory_map->interrupt_table)/sizeof(memory_map->interrupt_table[0]);
	
	for(int i = 0; i < inpterrupt_table_size; i++){
		interrput_vector = fetch_rom_data32(addr, start_rom);
		set_interrput_table(memory_map->interrupt_table, interrput_vector, i);
		addr += 4;
	}

	return SUCCESS;
}

/****** start the cpu. It will set to cpu->start ******/
error_code_t armcm3_startup(cpu_t* cpu)
{
	if(cpu == NULL || cpu->memory_map == NULL || cpu->ins_set == NULL){
		return ERROR_NULL_POINTER;
	}

	init_armcm3_interrput_table(cpu);

	armv7m_reg_t *regs = ((armv7m_instruct_t*)cpu->ins_set)->regs;
	memory_map_t* memory_map = cpu->memory_map;
	
	// set register initial value
	// TODO: Following statements need to be pack in another function which should be in armv7m module
	regs->MSP = memory_map->interrupt_table[0];
	regs->PC = align_address(memory_map->interrupt_table[1]);
	regs->xPSR = 0x0;
	SET_EPSR_T(regs, memory_map->interrupt_table[1] & BIT_0);
	/* if ESPR T is not zero, refer to B1-625 */

	regs->LR = 0xFFFFFFFF;
	regs->mode = MODE_THREAD;
	regs->memory = memory_map;

	return SUCCESS;
}



/****** fetch the instruction. It will set to cpu->fetch32 ******/
uint32_t fetch_armcm3_cpu(cpu_t* cpu)
{
	memory_map_t* memory_map = cpu->memory_map;
	uint32_t addr = ((armv7m_instruct_t*)cpu->ins_set)->regs->PC;

	return get_from_memory32(addr, memory_map);
}


/****** excute the cpu. It will set to cpu->excute ******/
void excute_armcm3_cpu(cpu_t* cpu, void* opcode){
	
	uint32_t opcode32 = *(uint32_t*)opcode;; 
	armv7m_reg_t *regs = ((armv7m_instruct_t*)cpu->ins_set)->regs;
	uint32_t PC_banked;

	/******							IMPROTANT									*/
	/****** PC always points to the address of next instruction.				*/
	/****** when 16bit coded, PC += 2. when 32bit coded, PC += 4.				*/		
	/****** But if instruction visits PC, it always returns PC+4				*/
	int i = 0;
	do{
		// if the code is encoded by 16 bit coding then parse the fist 16 bit and second 16 bit seperately 
		if(is_16bit_code(opcode32) == TRUE){
			regs->PC += 2;
			regs->PC_return = regs->PC + 2;
			PC_banked = regs->PC;
			parse_opcode16(opcode32, (armv7m_instruct_t*)cpu->ins_set);
			LOG_REG(regs);
			getchar();
			/* PC is modified by instruction */
			if(regs->PC != PC_banked){
				break;
			}
			opcode32 >>= 16;
			i++;

		// if the code is encoded by 32 bit coding then parse the 32 bit together 
		}else{
			/* the opcode is the low 16 bit but it seems to be a 32-bit instruction */
			if(i == 1){
				break;
			}
			regs->PC += 4;
			regs->PC_return = regs->PC;
			//parse_opcode32(opcode32, (armv7m_instruct_t*)cpu->ins_set);
		}
	}while(opcode32 != 0);
}


/****** Create an instance of the cpu. It will set to module->create ******/
cpu_t* create_armcm3_cpu()
{	
	LOG(LOG_DEBUG, "create_armcm3_cpu: create arm cortex m3 cpu.\n");
	
	// alloc memory for cpu
	cpu_t* cpu = alloc_cpu();

	// set cpu attributes
	//set_cpu_type(cpu, CPU_ARM_CM3);
	set_cpu_ins(cpu, create_armv7m_instruction());
	set_cpu_startup_func(cpu, armcm3_startup);
	set_cpu_fetch32_func(cpu, fetch_armcm3_cpu);
	set_cpu_exec_func(cpu, excute_armcm3_cpu);
	set_cpu_module(cpu, this_module);

	// add cpu to cpu list
	add_cpu_to_tail(this_module->cpu_list, cpu);

	return cpu;
}

error_code_t destory_armcm3_cpu(cpu_t** cpu)
{
	LOG(LOG_DEBUG, "destory_armcm3_cpu: destory arm cortex m3 cpu.\n");
	
	// delete from cpu list
	delete_cpu(this_module->cpu_list, *cpu);

	// destory
	destory_armv7m_instruction( (armv7m_instruct_t **)&(*cpu)->ins_set );
	dealloc_cpu(cpu);

	return SUCCESS;
}

/****** This is the unregister entrence used by main system ******/
error_code_t unregister_armcm3_module()
{
	if(registered == 0){
		return ERROR_REGISTERED;
	}

	LOG(LOG_DEBUG, "unregister_armcm3_module: unregister arm cortex m3 cpu.\n");
	unregister_module_helper(this_module);
	
	destory_module(&this_module);

	registered--;

	return SUCCESS;
}

/****** This is the register entrence used by main system ******/
error_code_t register_armcm3_module()
{
	error_code_t error_code;
	
	// Important the module can only be registered once
	if(registered == 1){
		return ERROR_REGISTERED;
	}
	
	LOG(LOG_DEBUG, "register_armcm3_module: register arm cortex m3 cpu.\n");
	// create module
	this_module = create_module();
	if(this_module == NULL){
		return ERROR_CREATE_MODULE;
	}


	// initialize module attributes
	set_module_type(this_module, MODULE_CPU);
	set_module_name(this_module, _T("arm_cm3"));

	// initialize module methods
	set_module_unregister(this_module, unregister_armcm3_module);
	set_module_content_create(this_module, (create_func_t)create_armcm3_cpu);
	set_module_content_destory(this_module, (destory_func_t)destory_armcm3_cpu);
	

	// register this module
	error_code = register_module_helper(this_module);
	if(error_code != SUCCESS){
		LOG(LOG_ERROR, "register_armcm3_module: can't regist module.\n");
	}
	
	registered++;

	return error_code;
}

