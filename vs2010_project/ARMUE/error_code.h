#ifndef _ERROR_CODE_H_
#define _ERROR_CODE_H_

typedef enum
{
	SUCCESS,
	ERROR_CREATE,
	ERROR_ADD,
	ERROR_CREATE_MODULE,
	ERROR_NULL_POINTER,
	ERROR_INVALID_PATH,
	ERROR_INVALID_MODULE,
	ERROR_INVALID_MODULE_PARAM,
	ERROR_DISMATCH_LIST,
	ERROR_REGISTERED,
	ERROR_INVALID_ROM_FILE,
	ERROR_CREATE_ROM_FILE,
	ERROR_ROM_PARAM_DISMATCH,
	ERROR_FETCH,
	ERROR_NO_START_ROM,
	ERROR_SOC_STARTUP,
}error_code_t;

#define LOG_ERROR			3
#define LOG_WARN			2
#define LOG_DEBUG			1
#define LOG_ALL				0
#define LOG_CURRENT_LEVEL	LOG_ALL

#include <stdio.h>


#define LOG(debug_level, message, ...) \
	if(debug_level >= LOG_CURRENT_LEVEL){\
		printf("[%s] "##message, #debug_level+4, __VA_ARGS__);\
	}

#ifdef _DEBUG
#define LOG_INSTRUCTION(message, ...)\
	printf("[INSTRUCTION] "##message, __VA_ARGS__);

#define LOG_REG(regs)\
		armv7m_print_reg_val(regs);
#else
#define LOG_INSTRUCTION(message, ...)
#define LOG_REG(regs)
#endif
#endif