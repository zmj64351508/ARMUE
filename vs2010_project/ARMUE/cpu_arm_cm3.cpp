#include "cpu.h"
#include "module_helper.h"
#include "error_code.h"
#include <stdlib.h>
#include "_types.h"
#include "arm_v7m_ins_decode.h"
#include "cm_NVIC.h"

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
	memory_region_t* region = find_address(memory_map, 0);
	if(region == NULL || region->type != MEMORY_REGION_ROM){
		return ERROR_NO_START_ROM;
	}
	rom_t* start_rom = (rom_t*)region->region_data;	

	uint32_t addr = 0;
	uint32_t interrput_vector = 0;
	int inpterrupt_table_size = cpu->cm_NVIC->vector_table_size;
	
	for(int i = 0; i < inpterrupt_table_size; i++){
		read_memory(addr, (uint8_t*)&interrput_vector, 4, memory_map);
		set_vector_table(cpu->cm_NVIC, interrput_vector, i);
		addr += 4;
	}
	cpu->cm_NVIC->throw_exception = cm_NVIC_throw_exception;
	cpu->cm_NVIC->check_exception = cm_NVIC_check_exception;
	cpu->cm_NVIC->handle_exception = cm_NVIC_handle_exception;

	return SUCCESS;
}

/****** start the cpu. It will set to cpu->start ******/
error_code_t armcm3_startup(cpu_t* cpu)
{
	if(cpu == NULL || cpu->memory_map == NULL){
		return ERROR_NULL_POINTER;
	}

	init_armcm3_interrput_table(cpu);

	armv7m_reg_t *regs = (armv7m_reg_t *)cpu->regs;
	memory_map_t* memory_map = cpu->memory_map;
	
	// set register initial value
	// TODO: Following statements need to be pack in another function which should be in armv7m module
	// reset behaviour refering to B1-642
	regs->MSP = get_vector_value(cpu->cm_NVIC, 0);
	regs->PC = align_address(get_vector_value(cpu->cm_NVIC, 1));
	regs->xPSR = 0x0;
	SET_EPSR_T(regs, get_vector_value(cpu->cm_NVIC, 1) & BIT_0);
	/* if ESPR T is not zero, refer to B1-625 */

	regs->LR = 0xFFFFFFFF;
	thumb_state* arm_state = (thumb_state*)cpu->run_info.cpu_spec_info;
	arm_state->mode = MODE_THREAD;

	return SUCCESS;
}

/****** fetch the instruction. It will set to cpu->fetch32 ******/
uint32_t fetch_armcm3_cpu(cpu_t* cpu)
{
	/* needn't fetch from memory */
	if(cpu->run_info.next_ins != 0){
		return (uint32_t)cpu->run_info.next_ins;
	}

	/* fetch opcode according to PC */
	memory_map_t* memory_map = cpu->memory_map;
	uint32_t addr = ((armv7m_reg_t*)cpu->regs)->PC;
	uint32_t opcode;
	read_memory(addr, (uint8_t*)&opcode, 4, memory_map);
	return opcode;
}

/* decode instruction, return the instruction info. It will be set to cpu->decode */
ins_t decode_armcm3_cpu(cpu_t* cpu, void* opcode)
{
	ins_t ins_info = {NULL, 0};
	uint32_t opcode32 = *(uint32_t*)opcode; 
	armv7m_reg_t *regs = (armv7m_reg_t *)cpu->regs;
	thumb_state *state = (thumb_state*)cpu->run_info.cpu_spec_info;

	/* decode the opcode. If opcode is 16bit coded, next_ins will store the next 16bit of this 32bit opcode
	   so that we don't need to read memory again to fetch opcode. When next_ins = 0, it indicates that no
	   more opcode is available and need to fetch code from memory */
	ins_info.opcode = opcode32;
	if(is_16bit_code(opcode32) == TRUE){
		ins_info.excute = thumb_parse_opcode16(opcode32, cpu);
		ins_info.length = 16;
		cpu->run_info.next_ins = opcode32 >> 16;
	}else{
		ins_info.length = 32;
		cpu->run_info.next_ins = 0;
		LOG(LOG_WARN, "32bit instruction: 0x%u\n", opcode32);
	}
	return ins_info;
}

/****** excute the cpu. It will set to cpu->excute ******/
void excute_armcm3_cpu(cpu_t* cpu, ins_t ins_info){
	
	armv7m_reg_t *regs = (armv7m_reg_t *)cpu->regs;
	thumb_state *state = (thumb_state*)cpu->run_info.cpu_spec_info;

	/******							IMPROTANT									*/
	/****** PC always points to the address of next instruction.				*/
	/****** when 16bit coded, PC += 2. when 32bit coded, PC += 4.				*/		
	/****** But if instruction visits PC, it always returns PC+4				*/
	armv7m_next_PC(cpu, ins_info.length);
	if(ins_info.length == 16){
		((thumb_translate16_t)ins_info.excute)((uint16_t)ins_info.opcode, cpu);
		if(armv7m_PC_modified(cpu)){
			cpu->run_info.next_ins = 0;
		}
	}else{
		// 32bit insturction
		goto out;
	}

	if(!check_and_reset_excuting_IT(state) && InITBlock(regs)){
		ITAdvance(regs);
	}

out:
	LOG_REG(regs);
	getchar();
}

/****** Create an instance of the cpu. It will set to module->create ******/
cpu_t* create_armcm3_cpu()
{	
	cpu_t* cpu = alloc_cpu();

	// set cpu attributes
	ins_thumb_init(cpu);
	cpu->startup = armcm3_startup;
	cpu->fetch32 = fetch_armcm3_cpu;
	cpu->decode = decode_armcm3_cpu;
	cpu->excute = excute_armcm3_cpu;
	set_cpu_module(cpu, this_module);

	// add cpu to cpu list
	add_cpu_to_tail(this_module->cpu_list, cpu);

	return cpu;
}

error_code_t destory_armcm3_cpu(cpu_t** cpu)
{
	ins_thumb_destory(*cpu);
	// delete from cpu list
	delete_cpu(this_module->cpu_list, *cpu);

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

	/* registered is the flag for whether this module is registered or not
	   Is there a better way to do something similar ? */
	registered--;

	return SUCCESS;
}

/****** This is the register entrence used by main system ******/
error_code_t register_armcm3_module()
{
	error_code_t error_code;
	
	/* Important the module can only be registered once
	   But this is not a good way. */	
	if(registered == 1){
		return ERROR_REGISTERED;
	}
	
	LOG(LOG_DEBUG, "register_armcm3_module: register arm cortex-m3 cpu.\n");
	// create module
	this_module = create_module();
	if(this_module == NULL){
		return ERROR_CREATE_MODULE;
	}


	// initialize module attributes
	set_module_type(this_module, MODULE_CPU);
	set_module_name(this_module, "arm_cm3");

	// initialize module methods
	set_module_unregister(this_module, unregister_armcm3_module);
	set_module_content_create(this_module, (create_func_t)create_armcm3_cpu);
	set_module_content_destory(this_module, (destory_func_t)destory_armcm3_cpu);
	
	// register this module
	error_code = register_module_helper(this_module);
	if(error_code != SUCCESS){
		LOG(LOG_ERROR, "register_armcm3_module: can't regist module.\n");
	}
	
	/* Well, as metioned, this should be improved. */
	registered++;

	return error_code;
}

