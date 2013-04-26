#ifndef _ERROR_CODE_H_
#define _ERROR_CODE_H_

typedef enum
{
	SUCCESS,
	ERROR_CREATE_MODULE,
	ERROR_NULL_POINTER,
	ERROR_INVALID_PATH,
	ERROR_INVALID_MODULE,
	ERROR_DISMATCH_LIST,
	ERROR_REGISTERED,
	ERROR_INVALID_ROM_FILE,
	ERROR_CREATE_ROM_FILE,
	ERROR_ROM_PARAM_DISMATCH,
	ERROR_FETCH,
	ERROR_NO_START_ROM,
	ERROR_SOC_STARTUP,
}error_code_t;

#define LOG_ERROR			4
#define LOG_WARN			3
#define LOG_DEBUG			2
#define LOG_INSTRUCTION		1
#define LOG_ALL				0
#define LOG_CURRENT_LEVEL	LOG_ALL

#include <stdio.h>
#define LOG(debug_level, message, ...) if(debug_level >= LOG_CURRENT_LEVEL){printf("[%s] "##message, #debug_level+4, __VA_ARGS__);}
#define LOG_REG(debug_level, regs) if(debug_level >= LOG_CURRENT_LEVEL){print_reg_val(regs);}

#endif